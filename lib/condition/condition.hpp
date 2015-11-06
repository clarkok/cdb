#ifndef _DB_CONDITION_CONDITION_H_
#define _DB_CONDITION_CONDITION_H_

#include <string>
#include <memory>

#include "lib/utils/buffer.hpp"
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

    struct FalseExpr : public ConditionExpr
    {
        virtual void accept(ConditionVisitor *v);
    };

    struct EvalExpr : public ConditionExpr
    {
        std::string column_name;

        EvalExpr(std::string column_name)
                : column_name(column_name)
        { }
    };

    struct CompareExpr : public EvalExpr
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

        Operator op;
        std::string literal;

        CompareExpr(std::string column_name, Operator op, std::string literal)
                : EvalExpr(column_name), op(op), literal(literal)
        { }

        virtual void accept(ConditionVisitor *v);
    };

    struct RangeExpr : public EvalExpr
    {
        std::string lower_value;
        std::string upper_value;

        RangeExpr(std::string column_name, std::string lower_value, std::string upper_value)
                : EvalExpr(column_name), lower_value(lower_value), upper_value(upper_value)
        { }

        virtual void accept(ConditionVisitor *v);
    };
}

#endif //_DB_CONDITON_CONDITION_H_
