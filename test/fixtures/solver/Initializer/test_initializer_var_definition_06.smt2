(set-logic QF_S)

(declare-fun abc () Int)
(declare-fun def () Number)
(declare-fun xyz () Bool)
(declare-fun klm () String)
(declare-fun prs () String)

(assert (= abc def))

(check-sat)

