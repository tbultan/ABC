/*
 * StringFormulaGenerator.cpp
 *
 *  Created on: Jan 22, 2017
 *      Author: baki
 */

#include "StringFormulaGenerator.h"

namespace Vlab {
namespace Solver {


using namespace SMT;
using namespace Theory;

const int StringFormulaGenerator::VLOG_LEVEL = 12;

/**
 * Generates atomic string formulae and formulae for boolean connectives
 *
 */
StringFormulaGenerator::StringFormulaGenerator(Script_ptr script, SymbolTable_ptr symbol_table,
                                                       ConstraintInformation_ptr constraint_information)
    : root_(script),
      symbol_table_(symbol_table),
      constraint_information_(constraint_information),
      has_mixed_constraint_{false} {

}

StringFormulaGenerator::~StringFormulaGenerator() {
  for (auto& el : term_formula_) {
    delete el.second;
  }

  for (auto& el : group_formula_) {
    delete el.second;
  }
}

void StringFormulaGenerator::start(Visitable_ptr node) {
  DVLOG(VLOG_LEVEL) << "String constraint extraction starts at node: " << node;
  visit(node);
  set_group_mappings();
  end();
}

void StringFormulaGenerator::start() {
  DVLOG(VLOG_LEVEL) << "String constraint extraction starts at root";
  visit(root_);
  set_group_mappings();
  end();
}

void StringFormulaGenerator::end() {
}

void StringFormulaGenerator::visitScript(Script_ptr script) {
  visit_children_of(script);
}

void StringFormulaGenerator::visitCommand(Command_ptr command) {
}

void StringFormulaGenerator::visitAssert(Assert_ptr assert_command) {
  visit_children_of(assert_command);
}

void StringFormulaGenerator::visitTerm(Term_ptr term) {
}

void StringFormulaGenerator::visitExclamation(Exclamation_ptr exclamation_term) {
}

void StringFormulaGenerator::visitExists(Exists_ptr exists_term) {
}

void StringFormulaGenerator::visitForAll(ForAll_ptr for_all_term) {
}

// TODO add formula generation for let scope
void StringFormulaGenerator::visitLet(Let_ptr let_term) {

}

void StringFormulaGenerator::visitAnd(And_ptr and_term) {
  DVLOG(VLOG_LEVEL) << "visit children start: " << *and_term << "@" << and_term;
  if (constraint_information_->is_component(and_term) and current_group_.empty()) {
    current_group_ = symbol_table_->get_var_name_for_node(and_term, Variable::Type::STRING);
    has_mixed_constraint_ = false;
  }
  visit_children_of(and_term);
  DVLOG(VLOG_LEVEL) << "visit children end: " << *and_term << "@" << and_term;

  if (not constraint_information_->is_component(and_term)) {
    current_group_ = "";
    has_mixed_constraint_ = false;
    return;
  }

  DVLOG(VLOG_LEVEL) << "post visit start: " << *and_term << "@" << and_term;

  auto group_formula = get_group_formula(current_group_);
  if (group_formula not_eq nullptr and group_formula->get_number_of_variables() > 0) {
    auto formula = group_formula->clone();
    formula->set_type(StringFormula::Type::INTERSECT);
    set_term_formula(and_term, formula);
    term_group_map_[and_term] = current_group_;
    constraint_information_->add_string_constraint(and_term);
    if (has_mixed_constraint_) {
      constraint_information_->add_mixed_constraint(and_term);
    }
  }

  DVLOG(VLOG_LEVEL) << "post visit end: " << *and_term << "@" << and_term;
}


void StringFormulaGenerator::visitOr(Or_ptr or_term) {
  DVLOG(VLOG_LEVEL) << "visit children start: " << *or_term << "@" << or_term;
  if (constraint_information_->is_component(or_term) and current_group_.empty()) {
    current_group_ = symbol_table_->get_var_name_for_node(or_term, Variable::Type::STRING);
    has_mixed_constraint_ = false;
  }
  visit_children_of(or_term);
  DVLOG(VLOG_LEVEL) << "visit children end: " << *or_term << "@" << or_term;

  // @deprecated check, all or terms must be a component
  // will be removed after careful testing
  if (not constraint_information_->is_component(or_term)) {
    current_group_ = "";
    has_mixed_constraint_ = false;
    return;
  }

  /**
   * If an or term does not have a child that has string formula, but we end up being here:
   * If or term does not have a group formula we are fine.
   * If or term has a group formula, that means or term is under a conjunction where other
   * conjunctive terms has string formula. We don't really need to set a formula for this
   * or term in this case (if has_string_formula var remains false).
   * ! Instead we can let it generate a formula for this or term  and handle this case in
   * String constraint solver by generating a Sigma* automaton, but this will be an
   * unneccessary intersection with other terms.
   */
  bool has_string_formula = false;
  for (auto term : *or_term->term_list) {
    has_string_formula = constraint_information_->has_string_constraint(term)
        or constraint_information_->has_mixed_constraint(term)
        or has_string_formula;
  }

  DVLOG(VLOG_LEVEL) << "post visit start: " << *or_term << "@" << or_term;
  auto group_formula = get_group_formula(current_group_);
  if (has_string_formula and group_formula not_eq nullptr and group_formula->get_number_of_variables() > 0) {
    auto formula = group_formula->clone();
    formula->set_type(StringFormula::Type::UNION);
    set_term_formula(or_term, formula);
    term_group_map_[or_term] = current_group_;
    constraint_information_->add_string_constraint(or_term);
    if (has_mixed_constraint_) {
      constraint_information_->add_mixed_constraint(or_term);
    }
  }
  DVLOG(VLOG_LEVEL) << "post visit end: " << *or_term << "@" << or_term;
}

void StringFormulaGenerator::visitNot(Not_ptr not_term) {
//  visit_children_of(not_term);
//  DVLOG(VLOG_LEVEL) << "post visit start: " << *not_term << "@" << not_term;
//
//  auto child_formula = get_term_formula(not_term->term);
//
//  if (child_formula not_eq nullptr) {
//    auto formula = child_formula->negate();
//    set_term_formula(not_term, formula);
//    term_group_map_[not_term] = current_group_;
//    if (string_terms_.size() > 0) {
//      string_terms_map_[not_term] = string_terms_;
//      string_terms_.clear();
//      constraint_information_->add_mixed_constraint(not_term);
//      has_mixed_constraint_ = true;
//    } else {
//      auto it = string_terms_map_.find(not_term->term);
//      if (it not_eq string_terms_map_.end()) {
//        string_terms_map_[not_term] = it->second;
//        string_terms_map_.erase(it);
//      }
//    }
//    constraint_information_->add_arithmetic_constraint(not_term);
//    term_group_map_.erase(not_term->term);
//    delete_term_formula(not_term->term);  // safe to call even there is no formula set
//  }
//  DVLOG(VLOG_LEVEL) << "post visit end: " << *not_term << "@" << not_term;
}

void StringFormulaGenerator::visitUMinus(UMinus_ptr u_minus_term) {
//  visit_children_of(u_minus_term);
//  DVLOG(VLOG_LEVEL) << "post visit start: " << *u_minus_term << "@" << u_minus_term;
//
//  auto child_formula = get_term_formula(u_minus_term->term);
//  auto formula = child_formula->Multiply(-1);
//  delete_term_formula(u_minus_term->term);
//  set_term_formula(u_minus_term, formula);
//
//  DVLOG(VLOG_LEVEL) << "post visit end: " << *u_minus_term << "@" << u_minus_term;
}

void StringFormulaGenerator::visitMinus(Minus_ptr minus_term) {
//  visit_children_of(minus_term);
//  DVLOG(VLOG_LEVEL) << "post visit start: " << *minus_term << "@" << minus_term;
//
//  auto left_formula = get_term_formula(minus_term->left_term);
//  auto right_formula = get_term_formula(minus_term->right_term);
//  auto formula = left_formula->Subtract(right_formula);
//  formula->set_type(StringFormula::Type::EQ);
//  delete_term_formula(minus_term->left_term);
//  delete_term_formula(minus_term->right_term);
//  set_term_formula(minus_term, formula);
//
//  DVLOG(VLOG_LEVEL) << "post visit end: " << *minus_term << "@" << minus_term;
}

void StringFormulaGenerator::visitPlus(Plus_ptr plus_term) {
//  DVLOG(VLOG_LEVEL) << "visit children start: " << *plus_term << "@" << plus_term;
//  StringFormula_ptr formula = nullptr,
//      plus_formula = nullptr,
//      param_formula = nullptr;
//  for (auto term_ptr : *(plus_term->term_list)) {
//    visit(term_ptr);
//    param_formula = get_term_formula(term_ptr);
//    if (formula == nullptr) {
//      formula = param_formula->clone();
//    } else {
//      plus_formula = formula->Add(param_formula);
//      delete formula;
//      formula = plus_formula;
//    }
//    delete_term_formula(term_ptr);
//  }
//  set_term_formula(plus_term, formula);
//
//  DVLOG(VLOG_LEVEL) << "visit children end: " << *plus_term << "@" << plus_term;
}

/**
 * All the parameters must be a constant integer except one.
 */
void StringFormulaGenerator::visitTimes(Times_ptr times_term) {
//  DVLOG(VLOG_LEVEL) << "visit children start: " << *times_term << "@" << times_term;
//  int multiplicant = 1;
//  StringFormula_ptr times_formula = nullptr;
//  for (auto term_ptr : *(times_term->term_list)) {
//    visit(term_ptr);
//    auto param_formula = get_term_formula(term_ptr);
//    if (param_formula->is_constant()) {
//      multiplicant = multiplicant * param_formula->get_constant();
//    } else if (times_formula == nullptr) {
//      times_formula = param_formula->clone();
//    } else {
//      LOG(FATAL)<< "Does not support non-linear multiplication";
//    }
//    delete_term_formula(term_ptr);
//  }
//  auto formula = times_formula->Multiply(multiplicant);
//  delete times_formula;
//  set_term_formula(times_term, formula);
//
//  DVLOG(VLOG_LEVEL) << "visit children end: " << *times_term << "@" << times_term;
}

// TODO make decision based on the formula type
void StringFormulaGenerator::visitEq(Eq_ptr eq_term) {
  visit_children_of(eq_term);
  DVLOG(VLOG_LEVEL) << "post visit start: " << *eq_term << "@" << eq_term;

  auto left_formula = get_term_formula(eq_term->left_term);
  auto right_formula = get_term_formula(eq_term->right_term);

  if (left_formula not_eq nullptr and right_formula not_eq nullptr) {
    if (StringFormula::Type::VAR == left_formula->get_type() and StringFormula::Type::VAR == right_formula->get_type()) {
      LOG(FATAL) << "implement me";
    } else {
      auto formula = left_formula->clone();
      formula->merge_variables(right_formula);
      formula->set_type(StringFormula::Type::NONRELATIONAL);
      delete_term_formula(eq_term->left_term);
      delete_term_formula(eq_term->right_term);
      set_term_formula(eq_term, formula);
      add_string_variables(current_group_, eq_term);
      has_mixed_constraint_ = true;
      constraint_information_->add_mixed_constraint(eq_term);
    }

//    auto formula = left_formula->Subtract(right_formula);
//    formula->set_type(StringFormula::Type::EQ);
//    delete_term_formula(eq_term->left_term);
//    delete_term_formula(eq_term->right_term);
//    set_term_formula(eq_term, formula);
//    add_string_variables(current_group_, eq_term);
//    if (string_terms_.size() > 0) {
//      formula->UpdateMixedConstraintRelations();
//      string_terms_map_[eq_term] = string_terms_;
//      string_terms_.clear();
//      constraint_information_->add_mixed_constraint(eq_term);
//      has_mixed_constraint_ = true;
//
//    }
//    constraint_information_->add_arithmetic_constraint(eq_term);
  } else if (left_formula not_eq nullptr and left_formula->get_number_of_variables() > 0) {
    auto formula = left_formula->clone();
    formula->set_type(StringFormula::Type::NONRELATIONAL);
    delete_term_formula(eq_term->left_term);
    set_term_formula(eq_term, formula);
    add_string_variables(current_group_, eq_term);
    has_mixed_constraint_ = true;
    constraint_information_->add_mixed_constraint(eq_term);
  } else if (right_formula not_eq nullptr and right_formula->get_number_of_variables() > 0) {
    auto formula = right_formula->clone();
    formula->set_type(StringFormula::Type::NONRELATIONAL);
    delete_term_formula(eq_term->left_term);
    set_term_formula(eq_term, formula);
    add_string_variables(current_group_, eq_term);
    has_mixed_constraint_ = true;
    constraint_information_->add_mixed_constraint(eq_term);
  }

  DVLOG(VLOG_LEVEL) << "post visit end: " << *eq_term << "@" << eq_term;
}

void StringFormulaGenerator::visitNotEq(NotEq_ptr not_eq_term) {
//  visit_children_of(not_eq_term);
//  DVLOG(VLOG_LEVEL) << "post visit start: " << *not_eq_term << "@" << not_eq_term;
//
//  auto left_formula = get_term_formula(not_eq_term->left_term);
//  auto right_formula = get_term_formula(not_eq_term->right_term);
//
//  if (left_formula not_eq nullptr and right_formula not_eq nullptr) {
//    auto formula = left_formula->Subtract(right_formula);
//    formula->set_type(StringFormula::Type::NOTEQ);
//    delete_term_formula(not_eq_term->left_term);
//    delete_term_formula(not_eq_term->right_term);
//    set_term_formula(not_eq_term, formula);
//    add_string_variables(current_group_, not_eq_term);
//    if (string_terms_.size() > 0) {
//      string_terms_map_[not_eq_term] = string_terms_;
//      string_terms_.clear();
//      constraint_information_->add_mixed_constraint(not_eq_term);
//      has_mixed_constraint_ = true;
//    }
//    constraint_information_->add_arithmetic_constraint(not_eq_term);
//  }
//  DVLOG(VLOG_LEVEL) << "post visit end: " << *not_eq_term << "@" << not_eq_term;
}

void StringFormulaGenerator::visitGt(Gt_ptr gt_term) {
//  visit_children_of(gt_term);
//  DVLOG(VLOG_LEVEL) << "post visit start: " << *gt_term << "@" << gt_term;
//
//  auto left_formula = get_term_formula(gt_term->left_term);
//  auto right_formula = get_term_formula(gt_term->right_term);
//
//  if (left_formula not_eq nullptr and right_formula not_eq nullptr) {
//    auto formula = left_formula->Subtract(right_formula);
//    formula->set_type(StringFormula::Type::GT);
//    delete_term_formula(gt_term->left_term);
//    delete_term_formula(gt_term->right_term);
//    set_term_formula(gt_term, formula);
//    add_string_variables(current_group_, gt_term);
//    if (string_terms_.size() > 0) {
//      string_terms_map_[gt_term] = string_terms_;
//      string_terms_.clear();
//      constraint_information_->add_mixed_constraint(gt_term);
//      has_mixed_constraint_ = true;
//    }
//    constraint_information_->add_arithmetic_constraint(gt_term);
//  }
//  DVLOG(VLOG_LEVEL) << "post visit end: " << *gt_term << "@" << gt_term;
}

void StringFormulaGenerator::visitGe(Ge_ptr ge_term) {
//  visit_children_of(ge_term);
//  DVLOG(VLOG_LEVEL) << "post visit start: " << *ge_term << "@" << ge_term;
//
//  auto left_formula = get_term_formula(ge_term->left_term);
//  auto right_formula = get_term_formula(ge_term->right_term);
//
//  if (left_formula not_eq nullptr and right_formula not_eq nullptr) {
//    auto formula = left_formula->Subtract(right_formula);
//    formula->set_type(StringFormula::Type::GE);
//    delete_term_formula(ge_term->left_term);
//    delete_term_formula(ge_term->right_term);
//    set_term_formula(ge_term, formula);
//    add_string_variables(current_group_, ge_term);
//    if (string_terms_.size() > 0) {
//      string_terms_map_[ge_term] = string_terms_;
//      string_terms_.clear();
//      constraint_information_->add_mixed_constraint(ge_term);
//      has_mixed_constraint_ = true;
//    }
//    constraint_information_->add_arithmetic_constraint(ge_term);
//  }
//  DVLOG(VLOG_LEVEL) << "post visit end: " << *ge_term << "@" << ge_term;
}

void StringFormulaGenerator::visitLt(Lt_ptr lt_term) {
//  visit_children_of(lt_term);
//  DVLOG(VLOG_LEVEL) << "post visit start: " << *lt_term << "@" << lt_term;
//
//  auto left_formula = get_term_formula(lt_term->left_term);
//  auto right_formula = get_term_formula(lt_term->right_term);
//
//  if (left_formula not_eq nullptr and right_formula not_eq nullptr) {
//    auto formula = left_formula->Subtract(right_formula);
//    formula->set_type(StringFormula::Type::LT);
//    delete_term_formula(lt_term->left_term);
//    delete_term_formula(lt_term->right_term);
//    set_term_formula(lt_term, formula);
//    add_string_variables(current_group_, lt_term);
//    if (string_terms_.size() > 0) {
//      string_terms_map_[lt_term] = string_terms_;
//      string_terms_.clear();
//      constraint_information_->add_mixed_constraint(lt_term);
//      has_mixed_constraint_ = true;
//    }
//    constraint_information_->add_arithmetic_constraint(lt_term);
//  }
//  DVLOG(VLOG_LEVEL) << "post visit end: " << *lt_term << "@" << lt_term;
}

void StringFormulaGenerator::visitLe(Le_ptr le_term) {
//  visit_children_of(le_term);
//  DVLOG(VLOG_LEVEL) << "post visit start: " << *le_term << "@" << le_term;
//
//  auto left_formula = get_term_formula(le_term->left_term);
//  auto right_formula = get_term_formula(le_term->right_term);
//
//  if (left_formula not_eq nullptr and right_formula not_eq nullptr) {
//    auto formula = left_formula->Subtract(right_formula);
//    formula->set_type(StringFormula::Type::LE);
//    delete_term_formula(le_term->left_term);
//    delete_term_formula(le_term->right_term);
//    set_term_formula(le_term, formula);
//    add_string_variables(current_group_, le_term);
//    if (string_terms_.size() > 0) {
//      string_terms_map_[le_term] = string_terms_;
//      string_terms_.clear();
//      constraint_information_->add_mixed_constraint(le_term);
//      has_mixed_constraint_ = true;
//    }
//    constraint_information_->add_arithmetic_constraint(le_term);
//  }
//  DVLOG(VLOG_LEVEL) << "post visit end: " << *le_term << "@" << le_term;
}

void StringFormulaGenerator::visitConcat(Concat_ptr concat_term) {
}


void StringFormulaGenerator::visitIn(In_ptr in_term) {
}


void StringFormulaGenerator::visitNotIn(NotIn_ptr not_in_term) {
}

void StringFormulaGenerator::visitLen(Len_ptr len_term) {
//  DVLOG(VLOG_LEVEL) << "visit: " << *len_term;
//
//  std::string name = symbol_table_->get_var_name_for_expression(len_term, Variable::Type::INT);
//
//  auto formula = new StringFormula();
//  formula->add_variable(name, 1);
//  formula->set_type(StringFormula::Type::VAR);
//  formula->add_relation_to_mixed_term(name, StringFormula::Type::NONE, len_term);
//
//  set_term_formula(len_term, formula);
//
//  string_terms_.push_back(len_term);
}

void StringFormulaGenerator::visitContains(Contains_ptr contains_term) {
  visit_children_of(contains_term);
  DVLOG(VLOG_LEVEL) << "post visit start: " << *contains_term << "@" << contains_term;

  auto left_formula = get_term_formula(contains_term->subject_term);
  auto right_formula = get_term_formula(contains_term->search_term);

  if (left_formula not_eq nullptr and right_formula not_eq nullptr) {
    auto formula = left_formula->clone();
    formula->merge_variables(right_formula);
    formula->set_type(StringFormula::Type::NONRELATIONAL);
    delete_term_formula(contains_term->subject_term);
    delete_term_formula(contains_term->subject_term);
    set_term_formula(contains_term, formula);
    add_string_variables(current_group_, contains_term);
    has_mixed_constraint_ = true;
    constraint_information_->add_mixed_constraint(contains_term);
  }

  DVLOG(VLOG_LEVEL) << "post visit end: " << *contains_term << "@" << contains_term;
}

void StringFormulaGenerator::visitNotContains(NotContains_ptr not_contains_term) {
  visit_children_of(not_contains_term);
  DVLOG(VLOG_LEVEL) << "post visit start: " << *not_contains_term << "@" << not_contains_term;

  auto left_formula = get_term_formula(not_contains_term->subject_term);
  auto right_formula = get_term_formula(not_contains_term->search_term);

  if (left_formula not_eq nullptr and right_formula not_eq nullptr) {
    auto formula = left_formula->clone();
    formula->merge_variables(right_formula);
    formula->set_type(StringFormula::Type::NONRELATIONAL);
    delete_term_formula(not_contains_term->subject_term);
    delete_term_formula(not_contains_term->subject_term);
    set_term_formula(not_contains_term, formula);
    add_string_variables(current_group_, not_contains_term);
    has_mixed_constraint_ = true;
    constraint_information_->add_mixed_constraint(not_contains_term);
  }

  DVLOG(VLOG_LEVEL) << "post visit end: " << *not_contains_term << "@" << not_contains_term;
}

void StringFormulaGenerator::visitBegins(Begins_ptr begins_term) {
}

void StringFormulaGenerator::visitNotBegins(NotBegins_ptr not_begins_term) {

}

void StringFormulaGenerator::visitEnds(Ends_ptr ends_term) {
}

void StringFormulaGenerator::visitNotEnds(NotEnds_ptr not_ends_term) {
}

void StringFormulaGenerator::visitIndexOf(IndexOf_ptr index_of_term) {
  visit_children_of(index_of_term);
  DVLOG(VLOG_LEVEL) << "post visit start: " << *index_of_term << "@" << index_of_term;

  auto left_formula = get_term_formula(index_of_term->subject_term);
  auto right_formula = get_term_formula(index_of_term->search_term);

  if (left_formula not_eq nullptr and right_formula not_eq nullptr) {
    auto formula = left_formula->clone();
    formula->merge_variables(right_formula);
    formula->set_type(StringFormula::Type::NONRELATIONAL);
    delete_term_formula(index_of_term->subject_term);
    delete_term_formula(index_of_term->subject_term);
    set_term_formula(index_of_term, formula);
  }

  DVLOG(VLOG_LEVEL) << "post visit end: " << *index_of_term << "@" << index_of_term;
}

void StringFormulaGenerator::visitLastIndexOf(LastIndexOf_ptr last_index_of_term) {
  visit_children_of(last_index_of_term);
  DVLOG(VLOG_LEVEL) << "post visit start: " << *last_index_of_term << "@" << last_index_of_term;

  auto left_formula = get_term_formula(last_index_of_term->subject_term);
  auto right_formula = get_term_formula(last_index_of_term->search_term);

  if (left_formula not_eq nullptr and right_formula not_eq nullptr) {
    auto formula = left_formula->clone();
    formula->merge_variables(right_formula);
    formula->set_type(StringFormula::Type::NONRELATIONAL);
    delete_term_formula(last_index_of_term->subject_term);
    delete_term_formula(last_index_of_term->subject_term);
    set_term_formula(last_index_of_term, formula);
  }

  DVLOG(VLOG_LEVEL) << "post visit end: " << *last_index_of_term << "@" << last_index_of_term;
}

void StringFormulaGenerator::visitCharAt(CharAt_ptr char_at_term) {
  visit_children_of(char_at_term);
  DVLOG(VLOG_LEVEL) << "post visit start: " << *char_at_term << "@" << char_at_term;

  auto left_formula = get_term_formula(char_at_term->subject_term);
  if (left_formula not_eq nullptr) {
    auto formula = left_formula->clone();
    formula->set_type(StringFormula::Type::NONRELATIONAL);
    delete_term_formula(char_at_term->subject_term);
    set_term_formula(char_at_term, formula);
  }

  DVLOG(VLOG_LEVEL) << "post visit end: " << *char_at_term << "@" << char_at_term;
}

void StringFormulaGenerator::visitSubString(SubString_ptr sub_string_term) {
  visit_children_of(sub_string_term);
  DVLOG(VLOG_LEVEL) << "post visit start: " << *sub_string_term << "@" << sub_string_term;

  auto left_formula = get_term_formula(sub_string_term->subject_term);
  if (left_formula not_eq nullptr) {
    auto formula = left_formula->clone();
    formula->set_type(StringFormula::Type::NONRELATIONAL);
    delete_term_formula(sub_string_term->subject_term);
    set_term_formula(sub_string_term, formula);
  }

  DVLOG(VLOG_LEVEL) << "post visit end: " << *sub_string_term << "@" << sub_string_term;
}

void StringFormulaGenerator::visitToUpper(ToUpper_ptr to_upper_term) {
}

void StringFormulaGenerator::visitToLower(ToLower_ptr to_lower_term) {
}

void StringFormulaGenerator::visitTrim(Trim_ptr trim_term) {
}

void StringFormulaGenerator::visitToString(ToString_ptr to_string_term) {
}

void StringFormulaGenerator::visitToInt(ToInt_ptr to_int_term) {
//  DVLOG(VLOG_LEVEL) << "visit: " << *to_int_term;
//
//  std::string name = symbol_table_->get_var_name_for_expression(to_int_term, Variable::Type::INT);
//
//  auto formula = new StringFormula();
//  formula->add_variable(name, 1);
//  formula->set_type(StringFormula::Type::VAR);
//  formula->add_relation_to_mixed_term(name, StringFormula::Type::NONE, to_int_term);
//
//  set_term_formula(to_int_term, formula);
//
//  string_terms_.push_back(to_int_term);
}

void StringFormulaGenerator::visitReplace(Replace_ptr replace_term) {
}

void StringFormulaGenerator::visitCount(Count_ptr count_term) {
}

void StringFormulaGenerator::visitIte(Ite_ptr ite_term) {
}

void StringFormulaGenerator::visitReConcat(ReConcat_ptr re_concat_term) {
}

void StringFormulaGenerator::visitReUnion(ReUnion_ptr re_union_term) {
}

void StringFormulaGenerator::visitReInter(ReInter_ptr re_inter_term) {
}

void StringFormulaGenerator::visitReStar(ReStar_ptr re_star_term) {
}

void StringFormulaGenerator::visitRePlus(RePlus_ptr re_plus_term) {
}

void StringFormulaGenerator::visitReOpt(ReOpt_ptr re_opt_term) {
}

void StringFormulaGenerator::visitToRegex(ToRegex_ptr to_regex_term) {
}

void StringFormulaGenerator::visitUnknownTerm(Unknown_ptr unknown_term) {
}

void StringFormulaGenerator::visitAsQualIdentifier(AsQualIdentifier_ptr as_qid_term) {
}

void StringFormulaGenerator::visitQualIdentifier(QualIdentifier_ptr qi_term) {
  DVLOG(VLOG_LEVEL) << "visit: " << *qi_term;

  Variable_ptr variable = symbol_table_->get_variable(qi_term->getVarName());
  if (Variable::Type::STRING == variable->getType()) {
    auto formula = new StringFormula();
    formula->add_variable(variable->getName(), 1);
    formula->set_type(StringFormula::Type::VAR);
    set_term_formula(qi_term, formula);
  }
}

void StringFormulaGenerator::visitTermConstant(TermConstant_ptr term_constant) {
  DVLOG(VLOG_LEVEL) << "visit: " << *term_constant;

  switch (term_constant->getValueType()) {
    case Primitive::Type::STRING: {
      auto formula = new StringFormula();
      formula->set_type(StringFormula::Type::STRING_CONSTANT);
      formula->set_constant(term_constant->getValue());
      set_term_formula(term_constant, formula);
      break;
    }
    case Primitive::Type::REGEX: {
      auto formula = new StringFormula();
      formula->set_type(StringFormula::Type::REGEX_CONSTANT);
      formula->set_constant(term_constant->getValue());
      set_term_formula(term_constant, formula);
      break;
    }
    default:
      break;
  }
}

void StringFormulaGenerator::visitIdentifier(Identifier_ptr identifier) {
}

void StringFormulaGenerator::visitPrimitive(Primitive_ptr primitive) {
}

void StringFormulaGenerator::visitTVariable(TVariable_ptr t_variable) {
}

void StringFormulaGenerator::visitTBool(TBool_ptr t_bool) {
}

void StringFormulaGenerator::visitTInt(TInt_ptr t_int) {
}

void StringFormulaGenerator::visitTString(TString_ptr t_string) {
}

void StringFormulaGenerator::visitVariable(Variable_ptr variable) {
}

void StringFormulaGenerator::visitSort(Sort_ptr sort) {
}

void StringFormulaGenerator::visitAttribute(Attribute_ptr attribute) {
}

void StringFormulaGenerator::visitSortedVar(SortedVar_ptr sorted_var) {
}

void StringFormulaGenerator::visitVarBinding(VarBinding_ptr var_binding) {
}

StringFormula_ptr StringFormulaGenerator::get_term_formula(Term_ptr term) {
  auto it = term_formula_.find(term);
  if (it == term_formula_.end()) {
    return nullptr;
  }
  return it->second;
}

StringFormula_ptr StringFormulaGenerator::get_group_formula(std::string group_name) {
  auto it = group_formula_.find(group_name);
  if (it == group_formula_.end()) {
    return nullptr;
  }
  return it->second;
}

bool StringFormulaGenerator::has_integer_terms(Term_ptr term) {
  return (integer_terms_map_.find(term) not_eq integer_terms_map_.end());
}

std::map<Term_ptr, TermList> StringFormulaGenerator::get_integer_terms_map() {
  return integer_terms_map_;
}

TermList& StringFormulaGenerator::get_integer_terms_in(Term_ptr term) {
  return integer_terms_map_[term];
}

void StringFormulaGenerator::clear_term_formula(Term_ptr term) {
  auto it = term_formula_.find(term);
  if (it != term_formula_.end()) {
    delete it->second;
    term_formula_.erase(it);
  }
}

void StringFormulaGenerator::clear_term_formulas() {
  for (auto& pair : term_formula_) {
    delete pair.second;
  }
  term_formula_.clear();
}

std::string StringFormulaGenerator::get_term_group_name(Term_ptr term) {
  auto it = term_group_map_.find(term);
  if (it not_eq term_group_map_.end()) {
    return it->second;
  }
  return "";
}

void StringFormulaGenerator::add_string_variables(std::string group_name, Term_ptr term) {
  StringFormula_ptr group_formula = nullptr;
  auto it = group_formula_.find(group_name);
  if (it == group_formula_.end()) {
    group_formula = new StringFormula();
    group_formula_[group_name] = group_formula;
  } else {
    group_formula = it->second;
  }
  auto formula = get_term_formula(term);
  group_formula->merge_variables(formula);
  // TODO if there is an or we need to add single variables into one group, uncomment above
  // line and remove the same line from else branch.
  // find a way to optimize that, if all constraints are nonrelational, we only need that
  // if we have disjunction
  if (StringFormula::Type::NONRELATIONAL == formula->get_type()) {
    clear_term_formula(term);
  } else {
//    group_formula->merge_variables(formula);
    term_group_map_[term] = group_name;
  }
}

bool StringFormulaGenerator::set_term_formula(Term_ptr term, StringFormula_ptr formula) {
  auto result = term_formula_.insert(std::make_pair(term, formula));
  if (result.second == false) {
    LOG(FATAL)<< "formula is already computed for term: " << *term;
  }
  return result.second;
}

void StringFormulaGenerator::delete_term_formula(Term_ptr term) {
  auto formula = get_term_formula(term);
  if (formula not_eq nullptr) {
    delete formula;
    term_formula_.erase(term);
  }
}

void StringFormulaGenerator::set_group_mappings() {
  DVLOG(VLOG_LEVEL)<< "start setting string group for components";
  for (auto& el : term_group_map_) {
    term_formula_[el.first]->merge_variables(group_formula_[el.second]);
  }
  // add a variable entry to symbol table for each group
  // define a variable mapping for a group
  for (auto& el : group_formula_) {
    symbol_table_->add_variable(new Variable(el.first, Variable::Type::NONE));
    for (const auto& var_entry : el.second->get_variable_coefficient_map()) {
      symbol_table_->add_variable_group_mapping(var_entry.first, el.first);
    }
  }
  DVLOG(VLOG_LEVEL)<< "end setting string group for components";
}

} /* namespace Solver */
} /* namespace Vlab */