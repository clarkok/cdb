#include "condition.hpp"
#include "column-name-visitor.hpp"

using namespace cdb;

void
ColumnNameVisitor::visit(AndExpr *expr)
{
    expr->lh->accept(this);
    expr->rh->accept(this);
}

void
ColumnNameVisitor::visit(OrExpr *expr)
{
    expr->lh->accept(this);
    expr->rh->accept(this);
}

void
ColumnNameVisitor::visit(CompareExpr *expr)
{ _column_names.insert(expr->column_name); }

void
ColumnNameVisitor::visit(RangeExpr *expr)
{ _column_names.insert(expr->column_name); }

void
ColumnNameVisitor::visit(FalseExpr *)
{ }

std::set<std::string>
ColumnNameVisitor::get() const
{ return _column_names; }
