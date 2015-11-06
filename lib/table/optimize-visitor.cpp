#include <lib/utils/comparator.hpp>
#include "optimize-visitor.hpp"
#include "lib/condition/condition.hpp"

using namespace cdb;

void
Table::OptimizeVisitor::visit(OrExpr *expr)
{
    _count = 0;
    expr->lh->accept(this);
    if (_replacement != expr->lh.get()) {
        expr->lh.reset(_replacement);
    }
    auto lh_count = _count;

    _count = 0;
    expr->rh->accept(this);
    if (_replacement != expr->rh.get()) {
        expr->rh.reset(_replacement);
    }
    auto rh_count = _count;

    if (dynamic_cast<FalseExpr*>(expr->lh.get())) {
        _replacement = expr->rh.release();
        _count = rh_count;
        return;
    }

    if (lh_count > rh_count) {
        std::swap(expr->lh, expr->rh);
    }

    _replacement = expr;
    _count = lh_count + rh_count;

    auto *lhe = dynamic_cast<EvalExpr*>(expr->lh.get()); auto *rhe = dynamic_cast<EvalExpr*>(expr->rh.get());

    if (!lhe || !rhe || lhe->column_name != rhe->column_name) {
        return;
    }

    if (!isRange(expr->lh.get()) || !isRange(expr->rh.get())) {
        return;
    }

    auto col = _owner->_schema->getColumnByName(lhe->column_name);
    auto type = col.getType();
    auto length = col.getField()->length;
    auto less = Comparator::getCompareFuncByTypeLT(type);

    auto *lhr = dynamic_cast<RangeExpr*>(lhe);
    auto *rhr = dynamic_cast<RangeExpr*>(rhe);

    auto lhr_lower = Convert::fromString(type, length, lhr->lower_value);
    auto lhr_upper = Convert::fromString(type, length, lhr->upper_value);
    auto rhr_lower = Convert::fromString(type, length, rhr->lower_value);
    auto rhr_upper = Convert::fromString(type, length, rhr->upper_value);

    if (
            less(rhr_upper.content(), lhr_lower.content()) &&
            less(lhr_upper.content(), rhr_lower.content())
    ) {
        auto new_lower = less(lhr_lower.content(), rhr_lower.content()) ? lhr_lower : rhr_lower;
        auto new_upper = less(lhr_upper.content(), rhr_upper.content()) ? rhr_upper : lhr_upper;

        _replacement = new RangeExpr(
                lhe->column_name,
                Convert::toString(type, new_lower),
                Convert::toString(type, new_upper)
        );
        _count = 1;
    }
}

void
Table::OptimizeVisitor::visit(AndExpr *expr)
{
    _count = 0;
    expr->lh->accept(this);
    if (dynamic_cast<FalseExpr*>(_replacement)) {
        _count = 0;
        return;
    }
    if (_replacement != expr->lh.get()) {
        expr->lh.reset(_replacement);
    }
    auto lh_count = _count;

    _count = 0;
    expr->rh->accept(this);
    if (dynamic_cast<FalseExpr*>(_replacement)) {
        _count = 0;
        return;
    }
    if (_replacement != expr->rh.get()) {
        expr->rh.reset(_replacement);
    }
    auto rh_count = _count;

    if (lh_count > rh_count) {
        std::swap(expr->lh, expr->rh);
    }

    _replacement = expr;
    _count = lh_count + rh_count;

    auto *lhe = dynamic_cast<EvalExpr*>(expr->lh.get());
    auto *rhe = dynamic_cast<EvalExpr*>(expr->rh.get());

    if (!lhe || !rhe || lhe->column_name != rhe->column_name) {
        return;
    }

    if (isRange(lhe) && isRange(rhe)) {
        auto col_name = lhe->column_name;
        auto col = _owner->_schema->getColumnByName(col_name);
        auto type = col.getType();
        auto length = col.getField()->length;
        auto less = Comparator::getCompareFuncByTypeLT(type);

        auto *lhr = dynamic_cast<RangeExpr*>(lhe);
        auto *rhr = dynamic_cast<RangeExpr*>(rhe);

        auto lhr_lower = Convert::fromString(type, length, lhr->lower_value);
        auto lhr_upper = Convert::fromString(type, length, lhr->upper_value);
        auto rhr_lower = Convert::fromString(type, length, rhr->lower_value);
        auto rhr_upper = Convert::fromString(type, length, rhr->upper_value);

        if (
                less(lhr_lower.content(), rhr_upper.content()) &&
                less(rhr_lower.content(), lhr_upper.content())
        ) {
            auto new_lower = less(lhr_lower.content(), rhr_lower.content()) ? rhr_lower : lhr_lower;
            auto new_upper = less(lhr_upper.content(), rhr_upper.content()) ? lhr_upper : rhr_upper;

            _replacement = new RangeExpr(
                    lhe->column_name,
                    Convert::toString(type, new_lower),
                    Convert::toString(type, new_upper)
            );
            _count = 1;
        }
        else {
            _replacement = new FalseExpr();
            _count = 0;
        }
    }
    else if (isRange(lhe)) {
        auto *rhc = dynamic_cast<CompareExpr*>(rhe);
        if (rhc->op != CompareExpr::Operator::EQ) {
            return;
        }

        auto col_name = lhe->column_name;
        auto col = _owner->_schema->getColumnByName(col_name);
        auto type = col.getType();
        auto length = col.getField()->length;
        auto less = Comparator::getCompareFuncByTypeLT(type);

        auto *lhr = dynamic_cast<RangeExpr*>(lhe);

        auto lhr_lower = Convert::fromString(type, length, lhr->lower_value);
        auto lhr_upper = Convert::fromString(type, length, lhr->upper_value);
        auto r_key = Convert::fromString(type, length, rhc->literal);

        if (
                !less(r_key.content(), lhr_lower.content()) &&
                 less(r_key.content(), lhr_upper.content())
        ) {
            _replacement = expr->lh.release();
            _count = 1;
        }
        else {
            _replacement = new FalseExpr();
            _count = 0;
        }
    }
    else if (isRange(rhe)) {
        auto *lhc = dynamic_cast<CompareExpr*>(lhe);
        if (lhc->op != CompareExpr::Operator::EQ) {
            return;
        }

        auto col_name = lhe->column_name;
        auto col = _owner->_schema->getColumnByName(col_name);
        auto type = col.getType();
        auto length = col.getField()->length;
        auto less = Comparator::getCompareFuncByTypeLT(type);

        auto *rhr = dynamic_cast<RangeExpr*>(rhe);

        auto rhr_lower = Convert::fromString(type, length, rhr->lower_value);
        auto rhr_upper = Convert::fromString(type, length, rhr->upper_value);
        auto l_key = Convert::fromString(type, length, lhc->literal);

        if (
                !less(l_key.content(), rhr_lower.content()) &&
                 less(l_key.content(), rhr_upper.content())
                ) {
            _replacement = expr->rh.release();
            _count = 1;
        }
        else {
            _replacement = new FalseExpr();
        }
    }
}

void
Table::OptimizeVisitor::visit(RangeExpr *expr)
{
    _replacement = expr;
    _count ++;
}

void
Table::OptimizeVisitor::visit(CompareExpr *expr)
{
    _replacement = expr;
    _count ++;

    auto col = _owner->_schema->getColumnByName(expr->column_name);
    auto type = col.getType();
    auto length = col.getField()->length;

    switch (expr->op) {
        case CompareExpr::Operator::EQ:
        case CompareExpr::Operator::NE:
            return;
        case CompareExpr::Operator::LT:
            _replacement = new RangeExpr(
                    expr->column_name,
                    Convert::toString(type, Convert::minLimit(type, length)),
                    expr->literal
            );
            return;
        case CompareExpr::Operator::LE:
            _replacement = new RangeExpr(
                    expr->column_name,
                    Convert::toString(type, Convert::minLimit(type, length)),
                    Convert::toString(
                            type,
                            Convert::next(
                                    type,
                                    length,
                                    Convert::fromString(type, length, expr->literal)))
            );
            return;
        case CompareExpr::Operator::GT:
            _replacement = new RangeExpr(
                    expr->column_name,
                    Convert::toString(
                            type,
                            Convert::next(
                                    type,
                                    length,
                                    Convert::fromString(type, length, expr->literal))),
                    Convert::toString(type, Convert::maxLimit(type, length))
            );
            return;
        case CompareExpr::Operator::GE:
            _replacement = new RangeExpr(
                    expr->column_name,
                    expr->literal,
                    Convert::toString(type, Convert::maxLimit(type, length))
            );
            return;
    }
}

void
Table::OptimizeVisitor::visit(FalseExpr *expr)
{ _replacement = expr; }

bool
Table::OptimizeVisitor::isRange(ConditionExpr *expr)
{
    auto range_expr = dynamic_cast<RangeExpr*>(expr);
    if (range_expr) {
        return true;
    }

    auto compare_expr = dynamic_cast<CompareExpr*>(expr);
    if (!compare_expr) {
        return false;
    }

    return (
            compare_expr->op != CompareExpr::Operator::EQ &&
            compare_expr->op != CompareExpr::Operator::NE
    );
}
