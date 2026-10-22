from __future__ import annotations

import hashlib
import json
import time
from dataclasses import asdict, dataclass
from pathlib import Path

from framework.common.config import load_config, load_dotenv, resolve_path


DEFAULT_MAX_TOKENS = 16000
NONSTREAMING_MAX_TOKENS = 21_333     # anthropic SDK refuses longer non-streaming requests (expected > 10 minutes)


class LLMError(RuntimeError):
    pass


@dataclass
class LLMResponse:
    text: str
    tokens_in: int
    tokens_out: int
    model_id: str
    latency_s: float
    stop_reason: str = ""
    cached: bool = False


class LLMClient:
    """generate(system, user) -> LLMResponse; replies are cached on disk by sha256(model, system, user)."""
    provider = "base"

    def __init__(self, model_cfg: dict, cache_dir: Path | None = None, sdk=None):
        self.model_id: str = model_cfg["id"]
        self.max_tokens = int(model_cfg.get("max_tokens", DEFAULT_MAX_TOKENS))
        self.effort = model_cfg.get("effort")
        try:
            self.price_in, self.price_out = float(model_cfg["price_in_per_mtok"]), float(model_cfg["price_out_per_mtok"])
        except (KeyError, ValueError):
            raise LLMError(f"model {self.model_id!r}: price_in_per_mtok / price_out_per_mtok must be numbers "
                           f"in configs/experiment.yaml") from None
        self.cache_dir = Path(cache_dir) if cache_dir else None
        self._sdk = sdk
        self.rep = 1          # repetition (experiment 2): rep >= 2 gets its own cache key, rep 1 keeps the old one

    @property
    def sdk(self):
        if self._sdk is None:
            load_dotenv()                       # API keys from .env, unless already in the environment
            self._sdk = self._make_sdk()
        return self._sdk

    def generate(self, system: str, user: str) -> LLMResponse:
        # the output limit joins the key only when it differs from the experiment 1 default, so old keys stay valid
        parts = (self.model_id, system, user) + ((f"#rep{self.rep}",) if self.rep > 1 else ()) \
            + ((f"#max{self.max_tokens}",) if self.max_tokens != DEFAULT_MAX_TOKENS else ())
        key = hashlib.sha256("\0".join(parts).encode("utf-8")).hexdigest()
        path = self.cache_dir / f"{key}.json" if self.cache_dir else None
        if path and path.exists():
            return LLMResponse(**json.loads(path.read_text(encoding="utf-8"))["response"] | {"cached": True})
        resp = self._call(system, user)
        if path:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(json.dumps({"response": asdict(resp), "system": system, "user": user}, indent=1),
                            encoding="utf-8")
        return resp

    def cost_usd(self, tokens_in: int, tokens_out: int) -> float:
        return (tokens_in * self.price_in + tokens_out * self.price_out) / 1e6

    def _make_sdk(self):
        raise NotImplementedError

    def _call(self, system: str, user: str) -> LLMResponse:
        raise NotImplementedError


class AnthropicClient(LLMClient):
    provider = "anthropic"

    def _make_sdk(self):
        import anthropic
        return anthropic.Anthropic(max_retries=3)

    def _call(self, system: str, user: str) -> LLMResponse:
        # claude-sonnet-5: no temperature/top_p/top_k/thinking (rejected); adaptive thinking is the default
        import anthropic
        extra = {"output_config": {"effort": self.effort}} if self.effort else {}
        t0 = time.time()
        kw = dict(model=self.model_id, max_tokens=self.max_tokens, system=system,
                  messages=[{"role": "user", "content": user}], **extra)
        try:
            if self.max_tokens > NONSTREAMING_MAX_TOKENS:            # experiment 2 limit: same reply, streamed
                with self.sdk.messages.stream(**kw) as stream:
                    msg = stream.get_final_message()
            else:
                msg = self.sdk.messages.create(**kw)
        except (TypeError, anthropic.AuthenticationError) as exc:   # missing (TypeError) or rejected key
            raise LLMError(f"Anthropic: {exc} (check ANTHROPIC_API_KEY in .env)") from exc
        text = "".join(b.text for b in msg.content if b.type == "text")
        return LLMResponse(text, msg.usage.input_tokens, msg.usage.output_tokens, self.model_id,
                           time.time() - t0, msg.stop_reason or "")


class OpenAIClient(LLMClient):
    provider = "openai"

    def _make_sdk(self):
        import openai
        try:
            return openai.OpenAI(max_retries=3)
        except openai.OpenAIError as exc:       # missing credentials
            raise LLMError(f"OpenAI: {exc} (set OPENAI_API_KEY in .env)") from exc

    def _call(self, system: str, user: str) -> LLMResponse:
        import openai
        extra = {"reasoning": {"effort": self.effort}} if self.effort else {}
        t0 = time.time()
        try:
            r = self.sdk.responses.create(model=self.model_id, instructions=system, input=user,
                                          max_output_tokens=self.max_tokens, **extra)
        except openai.AuthenticationError as exc:
            raise LLMError(f"OpenAI: {exc} (check OPENAI_API_KEY in .env)") from exc
        stop = r.incomplete_details.reason if r.status == "incomplete" else r.status
        return LLMResponse(r.output_text, r.usage.input_tokens, r.usage.output_tokens, self.model_id,
                           time.time() - t0, stop or "")


PROVIDERS = {"anthropic": AnthropicClient, "openai": OpenAIClient}


def make_client(model_id: str, cfg: dict | None = None, use_cache: bool = True, rep: int = 1,
                max_tokens: int | None = None) -> LLMClient:
    """max_tokens: overrides the model's limit (experiment2.max_tokens)."""
    cfg = cfg or load_config()
    model_cfg = next((m for m in cfg["models"] if m["id"] == model_id), None)
    if model_cfg is None or model_cfg.get("provider") not in PROVIDERS:
        raise LLMError(f"model {model_id!r} must be listed in configs/experiment.yaml with provider anthropic|openai")
    if max_tokens:
        model_cfg = {**model_cfg, "max_tokens": max_tokens}
    cache_dir = resolve_path(cfg["generation"]["llm_cache_dir"]) if use_cache else None
    client = PROVIDERS[model_cfg["provider"]](model_cfg, cache_dir)
    client.rep = rep
    return client
