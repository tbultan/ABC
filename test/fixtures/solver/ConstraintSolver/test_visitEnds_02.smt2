(set-logic QF_S)

(declare-fun var_abc () String)

(assert (ends var_abc /ab9+c/ ))

(check-sat)