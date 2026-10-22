# avr8js vs Wokwi crosscheck, experiment 2 (2026-10-12)

`python -m framework.harness.crosscheck` on the smoke tests of the 10 avr8js modules: 9/10 identical verdicts and counts (rerun after the last modules).

The one difference, `lcd_murasaki_dice` test `unconnected_pot_selects_d100`, reads the unconnected analog pin A0.
avr8js returns 0 for an unconnected input (deterministic, see framework/harness/README.md), Wokwi returns a floating
value (here selecting a d6). This is the documented floating-input difference of the harness, not a simulator bug;
the module runs on avr8js and its module.yaml states the test environment ("not connected in the test setup: reads
0"). All checks that do not read an unconnected input agree.
