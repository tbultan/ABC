/*
 * NaturalNumberAutomaton.h
 *
 *  Created on: Mar 14, 2018
 *      Author: baki
 */

#ifndef SRC_THEORY_NATURALNUMBERAUTOMATON_H_
#define SRC_THEORY_NATURALNUMBERAUTOMATON_H_

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <glog/logging.h>
#include <mona/bdd.h>
#include <mona/dfa.h>

#include "../utils/Cmd.h"
#include "../utils/List.h"
#include "ArithmeticFormula.h"
#include "Automaton.h"
#include "BinaryState.h"
#include "options/Theory.h"
#include "SemilinearSet.h"
#include "UnaryAutomaton.h"

namespace Vlab
{
  namespace Theory
  {

    class NaturalNumberAutomaton;
    using NaturalNumberAutomaton_ptr = NaturalNumberAutomaton*;

    class NaturalNumberAutomaton : public Automaton
    {
       public:

        /**
         * Binary encoded natural number automaton builder
         */
        class Builder;

        /**
         * Constructs a binary encoded natural number automaton given binary encoded dfa.
         * @param
         * @param num_of_variables
         */
        NaturalNumberAutomaton(const Libs::MONALib::DFA_ptr dfa, const int number_of_bdd_variables);

        /**
         * Constructs a binary encoded integer automaton given a binary encoded dfa and an arithmetic formula.
         * @param dfa
         * @param formula
         */
        NaturalNumberAutomaton(const Libs::MONALib::DFA_ptr dfa, const ArithmeticFormula_ptr formula);

        /**
         * Copy constructor.
         * @param
         */
        NaturalNumberAutomaton(const NaturalNumberAutomaton&);

        /**
         * Destructor.
         */
        virtual ~NaturalNumberAutomaton();

        /**
         * Generates a clone copy of the current automaton.
         * @return
         */
        virtual NaturalNumberAutomaton_ptr Clone() const override;

        /**
         * Generates a binary automaton that wraps the given dfa instance.
         * TODO need a parameter to specifiy integer or natural numbers; will separate into two classes.
         * @param dfa
         * @param number_of_variables
         * @return
         */
        virtual NaturalNumberAutomaton_ptr MakeAutomaton(Libs::MONALib::DFA_ptr dfa, const int number_of_variables) const
            override;

        /**
         * Prints the actual type and the details of the automaton.
         * @return
         */
        virtual std::string Str() const override;

        static NaturalNumberAutomaton_ptr MakePhi(ArithmeticFormula_ptr, bool is_natural_number);
        static NaturalNumberAutomaton_ptr MakeAnyInt(ArithmeticFormula_ptr, bool is_natural_number);
        static NaturalNumberAutomaton_ptr MakeAutomaton(ArithmeticFormula_ptr, bool is_natural_number);
        static NaturalNumberAutomaton_ptr MakeAutomaton(int value, std::string var_name, ArithmeticFormula_ptr formula,
                                                  bool add_leading_zeros = false);
        static NaturalNumberAutomaton_ptr MakeAutomaton(SemilinearSet_ptr semilinear_set, std::string var_name,
                                                  ArithmeticFormula_ptr formula, bool add_leading_zeros = false);

        ArithmeticFormula_ptr get_formula();
        void set_formula(ArithmeticFormula_ptr formula);
        bool is_natural_number();
        bool HasNegative1();
        NaturalNumberAutomaton_ptr Complement();
        NaturalNumberAutomaton_ptr Intersect(NaturalNumberAutomaton_ptr);
        NaturalNumberAutomaton_ptr Union(NaturalNumberAutomaton_ptr);
        NaturalNumberAutomaton_ptr Difference(NaturalNumberAutomaton_ptr);
        NaturalNumberAutomaton_ptr Exists(std::string var_name);
        NaturalNumberAutomaton_ptr GetBinaryAutomatonFor(std::string var_name);
        NaturalNumberAutomaton_ptr GetPositiveValuesFor(std::string var_name);
        NaturalNumberAutomaton_ptr GetNegativeValuesFor(std::string var_name);
        NaturalNumberAutomaton_ptr TrimLeadingZeros();
        NaturalNumberAutomaton_ptr AddLeadingZeros();
        SemilinearSet_ptr GetSemilinearSet();
        UnaryAutomaton_ptr ToUnaryAutomaton();

        std::map<std::string, int> GetAnAcceptingIntForEachVar();

        BigInteger SymbolicCount(double bound, bool count_less_than_or_equal_to_bound = false) override;

       protected:
        NaturalNumberAutomaton(ArithmeticFormula_ptr formula);
        static NaturalNumberAutomaton_ptr MakeIntGraterThanOrEqualToZero(std::vector<int> indexes, int number_of_variables);
        static NaturalNumberAutomaton_ptr MakeEquality(ArithmeticFormula_ptr, bool is_natural_number);
        static NaturalNumberAutomaton_ptr MakeIntEquality(ArithmeticFormula_ptr);
        static NaturalNumberAutomaton_ptr MakeNaturalNumberEquality(ArithmeticFormula_ptr);
        static NaturalNumberAutomaton_ptr MakeLessThan(ArithmeticFormula_ptr, bool is_natural_number);
        static NaturalNumberAutomaton_ptr MakeIntLessThan(ArithmeticFormula_ptr);
        static NaturalNumberAutomaton_ptr MakeNaturalNumberLessThan(ArithmeticFormula_ptr);
        static NaturalNumberAutomaton_ptr MakeLessThanOrEqual(ArithmeticFormula_ptr, bool is_natural_number);
        static NaturalNumberAutomaton_ptr MakeGreaterThan(ArithmeticFormula_ptr, bool is_natural_number);
        static NaturalNumberAutomaton_ptr MakeGreaterThanOrEqual(ArithmeticFormula_ptr, bool is_natural_number);
        static NaturalNumberAutomaton_ptr MakeTrimHelperAuto(int var_index, int number_of_variables);
        static void ComputeBinaryStates(std::vector<BinaryState_ptr>& binary_states, SemilinearSet_ptr semilinear_set);
        static void AddBinaryState(std::vector<BinaryState_ptr>& binary_states, std::vector<int>& constants);
        static int AddBinaryState(std::vector<BinaryState_ptr>& binary_states, int C, int R, BinaryState::Type t, int v,
                                  int b);
        static bool is_accepting_binary_state(BinaryState_ptr binary_state, SemilinearSet_ptr semilinear_set);

        void decide_counting_schema(Eigen::SparseMatrix<BigInteger>& count_matrix) override;

        bool GetCycleStatus(std::map<int, bool>& cycle_status);
        void GetCycleStatus(int state, std::map<int, int>& disc, std::map<int, int>& low, std::vector<int>& st,
                            std::map<int, bool>& is_stack_member, std::map<int, bool>& cycle_status, int& time);
//  bool getConstants(std::vector<int>& constants);
//  bool getConstants(int state, std::map<int, int>& disc, std::map<int, int>& low, std::vector<int>& st,
//          std::map<int, bool>& is_stack_member, std::vector<bool>& path, std::vector<int>& constants, int& time);
        void GetConstants(std::map<int, bool>& cycle_status, std::vector<int>& constants);
        void GetConstants(int state, std::map<int, bool>& cycle_status, std::vector<bool>& path,
                          std::vector<int>& constants);
        void GetBaseConstants(std::vector<int>& constants, unsigned max_number_of_bit_limit = 15);
        void GetBaseConstants(int state, unsigned char *is_visited, std::vector<bool>& path,
                              std::vector<int>& constants, unsigned max_number_of_bit_limit);
        //  void getBaseConstants2(std::vector<int>& constants);
        //  void getBaseConstants(int state, bool *is_stack_member, std::vector<bool>& path, std::vector<int>& constants);

        void add_print_label(std::ostream& out) override;
        struct StateIndices
        {
            // r suffixes are for the rejecting clone
            int i, ir;  // state index
            int s, sr;  // status: 0 not yet processed, 1 to be expanded, 2 done
            StateIndices()
                : i { -1 },
                  ir { -1 },
                  s { 0 },
                  sr { 0 }
            {
            }
        };

        ArithmeticFormula_ptr formula_;
       private:
        static const int VLOG_LEVEL;
    };

  } /* namespace Theory */
} /* namespace Vlab */

#endif /* SRC_THEORY_NATURALNUMBERAUTOMATON_H_ */