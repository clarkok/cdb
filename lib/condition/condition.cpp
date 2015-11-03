#include "condition.hpp"

using namespace cdb;

void
AndExpr::accept(ConditionVisitor *visitor)
{ visitor->visit(this); }

void
OrExpr::accept(ConditionVisitor *visitor)
{ visitor->visit(this); }

void
CompareExpr::accept(ConditionVisitor *visitor)
{ visitor->visit(this); }
