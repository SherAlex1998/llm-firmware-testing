from framework.common.protocol import parse_serial_log


def test_all_pass():
    run = parse_serial_log("@RUN_BEGIN 2\r\n@TEST a PASS\r\n@TEST b PASS\r\n@RUN_END 2/2\r\n")
    assert run.verdict == "pass" and (run.passed, run.total) == (2, 2)
    assert [t.name for t in run.tests] == ["a", "b"] and all(t.passed for t in run.tests)


def test_fail_with_message():
    run = parse_serial_log("@RUN_BEGIN 2\n@TEST a PASS\n@TEST b FAIL line 12: expected x == 1 (got 0 vs 1)\n@RUN_END 1/2\n")
    assert run.verdict == "fail"
    assert not run.tests[1].passed and run.tests[1].msg.startswith("line 12:")


def test_missing_run_end_is_timeout():
    run = parse_serial_log("@RUN_BEGIN 3\n@TEST a PASS\n@TEST b PASS\n")
    assert run.verdict == "timeout" and not run.run_end
    assert run.total == 3 and len(run.tests) == 2


def test_empty_log_is_timeout():
    assert parse_serial_log("").verdict == "timeout"


def test_garbage_ignored():
    run = parse_serial_log("\x00\xff boot\nhello\n@RUN_BEGIN 1\ndebug\n@TEST a PASS\nnoise @RUN_END 1/1 tail\n")
    assert run.verdict == "pass" and run.total == 1


def test_zero_tests_is_fail():
    assert parse_serial_log("@RUN_BEGIN 0\n@RUN_END 0/0\n").verdict == "fail"


def test_truncated_run_end_fail():
    run = parse_serial_log("@RUN_BEGIN 2\n@TEST a PASS\n@TEST b FAIL line 3: x\n@RUN_END")
    assert run.verdict == "fail" and (run.passed, run.total) == (1, 2)


def test_truncated_run_end_pass():
    run = parse_serial_log("@RUN_BEGIN 2\n@TEST a PASS\n@TEST b PASS\n@RUN_END ")
    assert run.verdict == "pass" and (run.passed, run.total) == (2, 2)


def test_warnings_collected():
    run = parse_serial_log("@RUN_BEGIN 1\n@TEST a PASS\n@WARN 2 tests over TEST_MAX_TESTS skipped\n@RUN_END 1/1\n")
    assert run.verdict == "pass" and run.warnings == ["2 tests over TEST_MAX_TESTS skipped"]


def test_declared_counts_win():
    run = parse_serial_log("@RUN_BEGIN 2\n@TEST a PASS\n@RUN_END 2/2\n")
    assert run.verdict == "pass" and run.passed == 2
