# Experiment 2 summary

Runs: 18 (3 modules, reps [1]); with errors: 0; total cost $3.31 (spent now $3.31); mutation skipped (rep outside mutation.reps): 0.

## complex (2 modules)

| model | rag | runs | compile@1 | compile@k | pass | mean tests | mutation score | cost $ |
|---|---|---|---|---|---|---|---|---|
| claude-haiku-4-5 | off | 2 | 0.50 | 1.00 | 0.00 | 22.0 | - | 0.103 |
| claude-haiku-4-5 | on | 2 | 0.50 | 0.50 | 0.00 | 16.0 | - | 0.134 |
| claude-sonnet-5 | off | 2 | 0.00 | 1.00 | 1.00 | 15.5 | 0.25 (n=2) | 1.373 |
| claude-sonnet-5 | on | 2 | 0.50 | 1.00 | 0.50 | 12.5 | 0.10 (n=1) | 0.835 |
| gpt-5.6-terra | off | 2 | 0.50 | 1.00 | 1.00 | 9.0 | 0.35 (n=2) | 0.208 |
| gpt-5.6-terra | on | 2 | 0.50 | 1.00 | 1.00 | 11.5 | 0.35 (n=2) | 0.233 |

## medium (1 modules)

| model | rag | runs | compile@1 | compile@k | pass | mean tests | mutation score | cost $ |
|---|---|---|---|---|---|---|---|---|
| claude-haiku-4-5 | off | 1 | 0.00 | 1.00 | 0.00 | 28.0 | - | 0.033 |
| claude-haiku-4-5 | on | 1 | 1.00 | 1.00 | 0.00 | 26.0 | - | 0.017 |
| claude-sonnet-5 | off | 1 | 1.00 | 1.00 | 1.00 | 17.0 | 0.50 (n=1) | 0.134 |
| claude-sonnet-5 | on | 1 | 1.00 | 1.00 | 0.00 | 15.0 | - | 0.143 |
| gpt-5.6-terra | off | 1 | 1.00 | 1.00 | 1.00 | 10.0 | 0.70 (n=1) | 0.046 |
| gpt-5.6-terra | on | 1 | 1.00 | 1.00 | 1.00 | 12.0 | 0.70 (n=1) | 0.052 |

## Secondary metrics

| tier | model | rag | share of correct tests | pruned mutation score |
|---|---|---|---|---|
| complex | claude-haiku-4-5 | off | 0.82 | 0.10 (n=2) |
| complex | claude-haiku-4-5 | on | 0.88 | 0.00 (n=1) |
| complex | claude-sonnet-5 | off | 1.00 | 0.25 (n=2) |
| complex | claude-sonnet-5 | on | 0.83 | 0.10 (n=2) |
| complex | gpt-5.6-terra | off | 1.00 | 0.35 (n=2) |
| complex | gpt-5.6-terra | on | 1.00 | 0.35 (n=2) |
| medium | claude-haiku-4-5 | off | 0.96 | 0.60 (n=1) |
| medium | claude-haiku-4-5 | on | 0.96 | 0.40 (n=1) |
| medium | claude-sonnet-5 | off | 1.00 | 0.50 (n=1) |
| medium | claude-sonnet-5 | on | 0.93 | 0.60 (n=1) |
| medium | gpt-5.6-terra | off | 1.00 | 0.70 (n=1) |
| medium | gpt-5.6-terra | on | 1.00 | 0.70 (n=1) |

Share of correct tests: tests_passed / tests_total of compiled runs. Pruned mutation score: the tests that failed on the original are disabled, the rest must pass, then the same 10 mutants run (equals the mutation score when the test passed).

## Wokwi

Logged runs 137, wall 830 s, retries 2, cloud errors 0; by kind: mutant 70 (317 s), mutant_pruned 55 (447 s), test 12 (65 s).

compile@1 / compile@k / pass / mutation score as in experiment 1; one row of results.parquet per (module, rag, model, rep).
