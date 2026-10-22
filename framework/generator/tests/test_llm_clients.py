from types import SimpleNamespace as NS

import pytest

from framework.generator.llm_clients import AnthropicClient, LLMClient, LLMError, LLMResponse, OpenAIClient, make_client

CFG = {"provider": "fake", "id": "fake-1", "max_tokens": 100, "price_in_per_mtok": 2.0, "price_out_per_mtok": 10.0}


class FakeClient(LLMClient):
    provider = "fake"

    def __init__(self, replies, cache_dir=None, model_id="fake-1"):
        super().__init__({**CFG, "id": model_id}, cache_dir)
        self.replies = list(replies)          # each live call pops one reply

    def _call(self, system, user):
        return LLMResponse(self.replies.pop(0), 10, 5, self.model_id, 0.1, "end_turn")


def test_cache_roundtrip(tmp_path):
    c = FakeClient(["A", "B"], tmp_path)
    r1 = c.generate("s", "u")
    assert (r1.text, r1.cached) == ("A", False)
    r2 = c.generate("s", "u")
    assert (r2.text, r2.cached, r2.tokens_in, r2.tokens_out) == ("A", True, 10, 5)
    r3 = c.generate("s", "u2")
    assert (r3.text, r3.cached) == ("B", False)
    assert c.replies == [] and len(list(tmp_path.glob("*.json"))) == 2


def test_cache_is_per_model(tmp_path):
    FakeClient(["A"], tmp_path).generate("s", "u")
    other = FakeClient(["B"], tmp_path, model_id="fake-2")
    assert other.generate("s", "u").text == "B"


def test_cache_disabled():
    c = FakeClient(["A", "B"])
    c.generate("s", "u")
    assert c.generate("s", "u").text == "B"


def test_cost():
    c = FakeClient([])
    assert c.cost_usd(1_000_000, 1_000_000) == 12.0
    assert c.cost_usd(500_000, 0) == 1.0


def test_bad_price_raises():
    with pytest.raises(LLMError, match="price"):
        LLMClient({"id": "x", "price_in_per_mtok": "PIN_ME", "price_out_per_mtok": 1})


def test_make_client_selects_provider():
    cfg = {"models": [{"provider": "anthropic", "id": "a", "price_in_per_mtok": 1, "price_out_per_mtok": 1},
                      {"provider": "openai", "id": "o", "price_in_per_mtok": 1, "price_out_per_mtok": 1},
                      {"provider": "azure", "id": "z", "price_in_per_mtok": 1, "price_out_per_mtok": 1}],
           "generation": {"llm_cache_dir": "results/.llm_cache"}}
    assert isinstance(make_client("a", cfg), AnthropicClient)
    assert isinstance(make_client("o", cfg), OpenAIClient)
    assert make_client("o", cfg, use_cache=False).cache_dir is None
    for bad in ("nope", "z"):
        with pytest.raises(LLMError):
            make_client(bad, cfg)


def test_anthropic_call_parses_text_blocks_and_omits_sampling():
    seen = {}

    def create(**kw):
        seen.update(kw)
        return NS(content=[NS(type="thinking", thinking="..."), NS(type="text", text="```cpp\nint a;\n```")],
                  usage=NS(input_tokens=11, output_tokens=7), stop_reason="end_turn")

    c = AnthropicClient({"provider": "anthropic", "id": "claude-sonnet-5", "max_tokens": 123,
                         "price_in_per_mtok": 3, "price_out_per_mtok": 15}, sdk=NS(messages=NS(create=create)))
    r = c.generate("SYS", "USR")
    assert r.text == "```cpp\nint a;\n```" and (r.tokens_in, r.tokens_out, r.stop_reason) == (11, 7, "end_turn")
    assert seen["model"] == "claude-sonnet-5" and seen["max_tokens"] == 123 and seen["system"] == "SYS"
    assert seen["messages"] == [{"role": "user", "content": "USR"}]
    assert not {"temperature", "top_p", "top_k", "thinking", "output_config"} & seen.keys()


def test_anthropic_effort_passthrough():
    seen = {}

    def create(**kw):
        seen.update(kw)
        return NS(content=[NS(type="text", text="x")], usage=NS(input_tokens=1, output_tokens=1), stop_reason="end_turn")

    c = AnthropicClient({"provider": "anthropic", "id": "claude-sonnet-5", "effort": "low",
                         "price_in_per_mtok": 3, "price_out_per_mtok": 15}, sdk=NS(messages=NS(create=create)))
    c.generate("s", "u")
    assert seen["output_config"] == {"effort": "low"}


def test_openai_call_reports_incomplete_reason():
    seen = {}

    def create(**kw):
        seen.update(kw)
        return NS(output_text="code", usage=NS(input_tokens=5, output_tokens=3), status="incomplete",
                  incomplete_details=NS(reason="max_output_tokens"))

    c = OpenAIClient({"provider": "openai", "id": "gpt-x", "max_tokens": 50, "price_in_per_mtok": 2,
                      "price_out_per_mtok": 12}, sdk=NS(responses=NS(create=create)))
    r = c.generate("SYS", "USR")
    assert r.text == "code" and (r.tokens_in, r.tokens_out, r.stop_reason) == (5, 3, "max_output_tokens")
    assert seen["model"] == "gpt-x" and seen["instructions"] == "SYS" and seen["input"] == "USR"
    assert seen["max_output_tokens"] == 50 and "temperature" not in seen and "reasoning" not in seen


def test_cache_key_includes_only_non_default_limit(tmp_path):
    import hashlib
    from framework.generator.llm_clients import DEFAULT_MAX_TOKENS
    c16 = FakeClient(["A"], tmp_path)
    c16.max_tokens = DEFAULT_MAX_TOKENS
    c16.generate("s", "u")
    old_key = hashlib.sha256("\0".join(("fake-1", "s", "u")).encode("utf-8")).hexdigest()
    assert (tmp_path / f"{old_key}.json").exists()               # experiment 1 keys unchanged
    c32 = FakeClient(["B"], tmp_path)
    c32.max_tokens = 32000
    assert c32.generate("s", "u").text == "B"                     # a cut-off 16000 reply is not reused


def test_anthropic_streams_long_limit():
    seen = {}

    class Stream:
        def __enter__(self):
            return self

        def __exit__(self, *a):
            return False

        def get_final_message(self):
            return NS(content=[NS(type="text", text="x")], usage=NS(input_tokens=2, output_tokens=3),
                      stop_reason="end_turn")

    def stream(**kw):
        seen.update(kw)
        return Stream()

    def create(**kw):
        raise AssertionError("a 32000-token request must be streamed")

    c = AnthropicClient({"provider": "anthropic", "id": "claude-sonnet-5", "max_tokens": 32000,
                         "price_in_per_mtok": 3, "price_out_per_mtok": 15},
                        sdk=NS(messages=NS(create=create, stream=stream)))
    r = c.generate("s", "u")
    assert (r.text, r.tokens_out, seen["max_tokens"]) == ("x", 3, 32000)


def test_make_client_max_tokens_override():
    assert make_client("claude-sonnet-5", use_cache=False).max_tokens == 16000
    assert make_client("claude-sonnet-5", use_cache=False, max_tokens=32000).max_tokens == 32000
