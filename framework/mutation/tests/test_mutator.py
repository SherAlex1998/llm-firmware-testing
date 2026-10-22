import pytest

from framework.common.config import load_config

from framework.mutation.mutator import find_mutation_sites, make_mutants
from framework.harness.sketch import module_sources
from framework.harness.tests.conftest import FIXTURE_DIR, requires_arduino


def sites(src, op):
    return [s for s in find_mutation_sites(src, [op])]


def only(src, op):
    found = sites(src, op)
    assert len(found) == 1, found
    return found[0].apply(src)


def test_flip_cmp_all_pairs():
    for a, b in [("==", "!="), ("!=", "=="), ("<", ">="), (">", "<="), ("<=", ">"), (">=", "<")]:
        assert only(f"bool f(int x) {{ return x {a} 3; }}\n", "flip_cmp") == f"bool f(int x) {{ return x {b} 3; }}\n"


def test_flip_cmp_ignores_shifts_arrows_includes_and_comments():
    src = '#include <Servo.h>\nint f(S* p) { return (1 << 3) + (p->v >> 1); } // a < b\n/* x > y */\n'
    assert sites(src, "flip_cmp") == []


def test_swap_hilo():
    assert only("void f() { digitalWrite(13, HIGH); }\n", "swap_hilo") == "void f() { digitalWrite(13, LOW); }\n"
    assert sites('const char* s = "HIGH"; // LOW\nint HIGHEST;\n', "swap_hilo") == []


def test_off_by_one_on_comparison_literal():
    assert only("bool f(int i) { return i < 5; }\n", "off_by_one") == "bool f(int i) { return i < 6; }\n"
    assert sites("int f() { return (1 << 3) + 2.5 + 0x10 + 7UL; }\n", "off_by_one") == []


def test_const_change_on_arguments_and_initializers():
    assert only("void f() { analogWrite(9, x); }\n", "const_change") == "void f() { analogWrite(18, x); }\n"
    assert only("void f() { g(0); }\n", "const_change") == "void f() { g(1); }\n"
    assert [s.mutated for s in sites("const int P[] = {2, 3};\n", "const_change")] == ["4", "6"]
    assert sites("void f() { g(x + 1); h(2 * y); }\n", "const_change") == []


def test_drop_call_statements_only():
    src = "void f() {\n  led_on();\n  s.write(90);\n  delay(t);\n  pinMode(13, OUTPUT);\n  if (x) y();\n  return g();\n}\n"
    dropped = [s.original for s in sites(src, "drop_call")]
    assert dropped == ["led_on();", "s.write(90);"]
    assert sites(src, "drop_call")[0].apply(src).splitlines()[1] == "  ;"


def test_negate_cond_balanced_parens():
    src = "void f() { if (g(a) && (b > 1)) h(); while (k) {} }\n"
    assert [s.mutated for s in sites(src, "negate_cond")] == ["!(g(a) && (b > 1))", "!(k)"]


def test_swap_logic():
    assert only("bool f(bool a, bool b) { return a && b; }\n", "swap_logic") == "bool f(bool a, bool b) { return a || b; }\n"
    assert only("bool f(bool a, bool b) { return a || b; }\n", "swap_logic") == "bool f(bool a, bool b) { return a && b; }\n"
    assert sites("int f(int a) { return a & 1 | 2; } // a && b\n", "swap_logic") == []


def test_swap_arith_binary_operators():
    for a, b in [("+", "-"), ("-", "+"), ("*", "/"), ("/", "*"), ("%", "/")]:
        assert only(f"int f(int x) {{ return (x) {a} 3; }}\n", "swap_arith") == f"int f(int x) {{ return (x) {b} 3; }}\n"
    assert only("int f(int a[]) { return a[1]*2; }\n", "swap_arith") == "int f(int a[]) { return a[1]/2; }\n"


def test_swap_arith_skips_unary_increments_compound_and_arrows():
    src = "int f(S* p, int x) {\n  x++; --x; x += 2; x -= 1;\n  int y = -x;\n  if (x < -1) return -1;\n  return p->v;\n}\n"
    assert [s.original for s in sites(src, "swap_arith") if s.original != "*"] == []


def test_only_one_change_per_site():
    src = module_sources(FIXTURE_DIR)["logic.cpp"]
    for s in find_mutation_sites(src):
        out = s.apply(src)
        assert out != src and out[:s.start] == src[:s.start] and out[s.start + len(s.mutated):] == src[s.end:]


def test_make_mutants_deterministic_and_diverse(tmp_path):
    always = lambda *a: True
    a = make_mutants("blink_fixture", 5, seed=7, work_dir=tmp_path, check=always)
    b = make_mutants("blink_fixture", 5, seed=7, work_dir=tmp_path, check=always)
    c = make_mutants("blink_fixture", 5, seed=8, work_dir=tmp_path, check=always)
    assert [m.site for m in a] == [m.site for m in b] != [m.site for m in c]
    assert len(a) == 5 and [m.id for m in a] == [1, 2, 3, 4, 5]
    ops = [m.site.operator for m in a]
    assert max(ops.count(o) for o in ops) <= 2 and len({m.site.line for m in a}) == 5
    assert all(m.before != m.after and m.source != module_sources(FIXTURE_DIR)["logic.cpp"] for m in a)


def test_make_mutants_skips_non_compiling(tmp_path):
    rejected = []

    def check(module_dir, src, work, file="logic.cpp"):
        if len(rejected) < 2:
            rejected.append(src)
            return False
        return True

    ms = make_mutants("blink_fixture", 5, seed=1, work_dir=tmp_path, check=check)
    assert len(ms) == 5 and not {m.source for m in ms} & set(rejected)


def test_make_mutants_fills_up_when_spread_is_not_enough(tmp_path):
    ms = make_mutants("blink_fixture", 20, seed=1, operators=["swap_hilo", "flip_cmp"], work_dir=tmp_path,
                      check=lambda *a: True)
    assert len(ms) == 6                              # all 4 swap_hilo + 2 flip_cmp sites of the fixture


@requires_arduino
@pytest.mark.arduino
def test_real_mutants_compile(tmp_path):
    ms = make_mutants("blink_fixture", work_dir=tmp_path)
    assert len(ms) == load_config()["mutation"]["mutants_per_module"]
