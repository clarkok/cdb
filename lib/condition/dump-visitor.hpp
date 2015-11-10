#ifndef _DB_CONDITION_DUMP_VISITOR_H_
#define _DB_CONDITION_DUMP_VISITOR_H_

#include <iostream>
#include "visitor.hpp"

namespace cdb {
    class ConditionDumpVisitor : public ConditionVisitor
    {
        std::ostream &os;
    public:
        ConditionDumpVisitor(std::ostream &os);
        virtual ~ConditionDumpVisitor() = default;
        virtual void visit(AndExpr *);
        virtual void visit(OrExpr *);
        virtual void visit(CompareExpr *);
        virtual void visit(RangeExpr *);
        virtual void visit(FalseExpr *);
    };
}

#endif // _DB_CONDITION_DUMP_VISITOR_H_
