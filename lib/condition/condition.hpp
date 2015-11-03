#ifndef _DB_CONDITION_CONDITION_H_
#define _DB_CONDITION_CONDITION_H_

#include <string>
#include <memory>

#include "visitor.hpp"

namespace cdb {
    class ConditionVisitor;

    struct ConditionExpr
    {
        virtual ~ConditionExpr() = default;

        virtual void accept(ConditionVisitor *visitor) = 0;
    };

    struct AndExpr : public ConditionExpr
    {
        std::unique_ptr<ConditionExpr> lh;
        std::unique_ptr<ConditionExpr> rh;

        AndExpr(ConditionExpr *lh, ConditionExpr *rh)
                : lh(lh), rh(rh)
        { }
        virtual ~AndExpr() = default;

        virtual void accept(ConditionVisitor *v);
    };

    struct OrExpr : public ConditionExpr
    {
        std::unique_ptr<ConditionExpr> lh;
        std::unique_ptr<ConditionExpr> rh;

        OrExpr(ConditionExpr *lh, ConditionExpr *rh)
                : lh(lh), rh(rh)
        { }
        virtual ~OrExpr() = default;

        virtual void accept(ConditionVisitor *v);
    };

    struct CompareExpr : public ConditionExpr
    {
        enum class Operator
        {
            EQ,
            NE,
            GE,
            LE,
            GT,
            LT
        };

        std::string column_name;
        Operator op;
        std::string literal;

        CompareExpr(std::string column_name, Operator op, std::string literal)
                : column_name(column_name), op(op), literal(literal)
        { }

        virtual void accept(ConditionVisitor *v);
    };
}

#endif //_DB_CONDITON_CONDITION_H_
