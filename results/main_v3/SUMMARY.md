# Experiment summary

Runs: 90; with errors: 0; total cost $2.10 (spent now $0.00); tokens in/out 366822/118546.

| model | rag | runs | compile@1 | compile@k | pass | mean tests | mutation score | cost $ |
|---|---|---|---|---|---|---|---|---|
| claude-haiku-4-5 | off | 15 | 0.93 | 1.00 | 0.60 | 8.3 | 0.81 (n=9) | 0.127 |
| claude-haiku-4-5 | on | 15 | 1.00 | 1.00 | 0.40 | 8.5 | 0.83 (n=6) | 0.159 |
| claude-sonnet-5 | off | 15 | 1.00 | 1.00 | 0.87 | 6.9 | 0.78 (n=13) | 0.557 |
| claude-sonnet-5 | on | 15 | 1.00 | 1.00 | 0.87 | 7.0 | 0.81 (n=13) | 0.630 |
| gpt-5.6-terra | off | 15 | 1.00 | 1.00 | 0.87 | 6.2 | 0.85 (n=13) | 0.282 |
| gpt-5.6-terra | on | 15 | 1.00 | 1.00 | 1.00 | 6.7 | 0.88 (n=15) | 0.348 |

compile@1: compiled without repair; compile@k: compiled after at most 2 repairs; pass: every test passed on the original module; mutation score: mean over runs that passed.
