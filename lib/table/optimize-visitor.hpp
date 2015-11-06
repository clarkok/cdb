#ifndef _DB_TABLE_OPTIMIZE_VISITOR_H_
#define _DB_TABLE_OPTIMIZE_VISITOR_H_

#include "lib/condition/visitor.hpp"
#include "table.hpp"

namespace cdb {
    class Table::OptimizeVisitor : public ConditionVisitor
    {
        Table *_owner;

        ConditionExpr *_replacement = nullptr;
        int _count = 0;

        bool isRange(ConditionExpr *expr);

    public:
        OptimizeVisitor(Table *owner)
                : _owner(owner)
        { }

        virtual void visit(OrExpr *expr);
        virtual void visit(AndExpr *expr);
        virtual void visit(CompareExpr *expr);
        virtual void visit(RangeExpr *expr);
        virtual void visit(FalseExpr *expr);

        ConditionExpr *
        get() const
        { return _replacement; }
    };
}

#endif // _DB_TABLE_OPTIMIZE_VISITOR_H_
