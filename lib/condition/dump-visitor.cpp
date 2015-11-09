#include "dump-visitor.hpp"

using namespace cdb;

ConditionDumpVisitor::ConditionDumpVisitor(std::ostream &os)
    : os(os)
{ }

void
ConditionDumpVisitor::visit(AndExpr *expr)
{
    os << "AND" << std::endl;
    expr->lh->accept(this);
    expr->rh->accept(this);
}

void
ConditionDumpVisitor::visit(OrExpr *expr)
{
    os << "OR" << std::endl;
    expr->lh->accept(this);
    expr->rh->accept(this);
}

void
ConditionDumpVisitor::visit(CompareExpr *expr)
{
    os << "Compare " << expr->column_name;
    switch (expr->op) {
        case CompareExpr::Operator::EQ:
            os << " = ";
            break;
        case CompareExpr::Operator::NE:
            os << " <> ";
            break;
        case CompareExpr::Operator::GE:
            os << " >= ";
            break;
        case CompareExpr::Operator::GT:
            os << " > ";
            break;
        case CompareExpr::Operator::LE:
            os << " <= ";
            break;
        case CompareExpr::Operator::LT:
            os << " < ";
            break;
    }
    os << expr->literal << std::endl;
}

void
ConditionDumpVisitor::visit(RangeExpr *expr)
{
    os << "Range " << expr->column_name << " in [" 
        << expr->lower_value << ", " << expr->upper_value << ")" << std::endl;
}

void
ConditionDumpVisitor::visit(FalseExpr *)
{
    os << "False" << std::endl;
}
