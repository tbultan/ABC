/*
 * SymbolTable.cpp
 *
 *  Created on: Apr 27, 2015
 *      Author: baki
 */

#include "SymbolTable.h"

namespace Vlab {
namespace Solver {

using namespace SMT;

const int SymbolTable::VLOG_LEVEL = 10;

SymbolTable::SymbolTable()
        : global_assertion_result(true), bound(50) {
}
SymbolTable::~SymbolTable() {

  reset_substitution_rules();

  for (auto& map_pair : variable_value_table) {
    for (auto& value_pair : map_pair.second) {
      delete value_pair.second;
    }
  }
  variable_value_table.clear();

  for (auto& entry : variables) {
    delete entry.second;
  }
}

bool SymbolTable::isSatisfiable() {
  return global_assertion_result;
}

void SymbolTable::updateSatisfiability(bool value) {
  global_assertion_result = global_assertion_result and value;
}

void SymbolTable::addVariable(Variable_ptr variable) {
  auto result = variables.insert(std::make_pair(variable->getName(), variable));
  if (not result.second) {
    LOG(FATAL)<< "Duplicate variable definition: " << *variable;
  }
}

Variable_ptr SymbolTable::getVariable(std::string name) {
  auto it = variables.find(name);
  CHECK(it != variables.end()) << "Variable is not found: " << name;
  return it->second;
}

Variable_ptr SymbolTable::getVariable(Term_ptr term_ptr) {
  QualIdentifier_ptr variable_identifier = dynamic_cast<QualIdentifier_ptr>(term_ptr);
  CHECK_NOTNULL(variable_identifier);
  return getVariable(variable_identifier->getVarName());
}

VariableMap& SymbolTable::getVariables() {
  return variables;
}

SMT::Variable_ptr SymbolTable::getSymbolicVariable() {
  auto it = std::find_if(variables.begin(), variables.end(),
      [](std::pair<std::string, SMT::Variable_ptr> entry) -> bool {
        return entry.second->isSymbolic();
  });
  if (it != variables.end()) {
    return it->second;
  }
  LOG(FATAL) << "no symbolic variable found";
  return nullptr;
}

void SymbolTable::setBound(int bound) {
  this->bound = bound;
}

int SymbolTable::getBound() {
  return bound;
}

void SymbolTable::push_scope(Visitable_ptr key) {
  scope_stack.push_back(key);
}

Visitable_ptr SymbolTable::pop_scope() {
  Visitable_ptr scope = scope_stack.back();
  scope_stack.pop_back();
  return scope;
}


void SymbolTable::increment_count(Variable_ptr variable) {
  variable_counts_table[scope_stack.back()][variable]++;
}

void SymbolTable::decrement_count(Variable_ptr variable) {
  variable_counts_table[scope_stack.back()][variable]--;
}

int SymbolTable::get_local_count(Variable_ptr variable) {
  return variable_counts_table[scope_stack.back()][variable];
}

int SymbolTable::get_total_count(Variable_ptr variable) {
  int total = 0;
  for (auto& counters : variable_counts_table) {
    total += counters.second[variable];
  }
  return total;
}
void SymbolTable::reset_count() {
  variable_counts_table.clear();
}

bool SymbolTable::add_var_substitution_rule(Variable_ptr variable, Term_ptr target_term) {
  auto result = variable_substitution_table[scope_stack.back()].insert(std::make_pair(variable, target_term));
  return result.second;
}

bool SymbolTable::remove_var_substitution_rule(Variable_ptr variable) {
  auto it = variable_substitution_table[scope_stack.back()].find(variable);
  if (it != variable_substitution_table[scope_stack.back()].end()) {
    variable_substitution_table[scope_stack.back()].erase(it);
    return true;
  }
  return false;
}

Term_ptr SymbolTable::get_variable_substitution_term(Variable_ptr variable) {
  auto it = variable_substitution_table[scope_stack.back()].find(variable);
  if (it == variable_substitution_table[scope_stack.back()].end()) {
    return nullptr;
  }
  return it->second;
}

/**
 * Returns rules for the current scope
 */
VariableSubstitutionMap& SymbolTable::get_variable_substitution_map() {
  return variable_substitution_table[scope_stack.back()];
}

/**
 * Returns rules within all scopes
 */
VariableSubstitutionTable& SymbolTable::get_variable_substitution_table() {
  return variable_substitution_table;
}

void SymbolTable::reset_substitution_rules() {
  for (auto& map_pair : variable_substitution_table) {
    for (auto& rule_pair : map_pair.second) {
      delete rule_pair.second;
    }
  }
  variable_substitution_table.clear();
}

Value_ptr SymbolTable::getValue(std::string var_name) {
  return getValue(getVariable(var_name));
}

Value_ptr SymbolTable::getValue(SMT::Variable_ptr variable) {
  Value_ptr result = nullptr;

  for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); it++) {
    auto entry = variable_value_table[(*it)].find(variable);
    if (entry != variable_value_table[(*it)].end()) {
      return entry->second;
    }
  }

  switch (variable->getType()) {
  case Variable::Type::BOOL:
    LOG(FATAL) << "implement me";
    break;
  case Variable::Type::INT:
    result = new Value(Value::Type::INT_AUTOMATON, Theory::IntAutomaton::makeAnyInt());
    DVLOG(VLOG_LEVEL) << "initialized variable as any integer: " << *variable;
    break;
  case Variable::Type::STRING:
    result = new Value(Value::Type::STRING_AUTOMATON, Theory::StringAutomaton::makeAnyString());
    DVLOG(VLOG_LEVEL) << "initialized variable as any string: " << *variable;
    break;
  default:
    LOG(FATAL) << "unknown variable type" << *variable;
    break;
  }
  setValue(variable, result);
  return result;
}

bool SymbolTable::setValue(std::string var_name, Value_ptr value) {
return setValue(getVariable(var_name), value);
}

bool SymbolTable::setValue(SMT::Variable_ptr variable, Value_ptr value) {
//  auto result = variable_value_table[scope_stack.back()].insert(std::make_pair(variable, value));
//  return result.second;
  variable_value_table[scope_stack.back()][variable] = value;
  return true;
}

} /* namespace Solver */
} /* namespace Vlab */

