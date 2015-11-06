#ifndef _DB_CONDITION_COLUMN_NAME_VISITOR_H_
#define _DB_CONDITION_COLUMN_NAME_VISITOR_H_

#include <set>
#include <string>
#include "visitor.hpp"

namespace cdb {
    class ColumnNameVisitor : public ConditionVisitor
    {
        std::set<std::string> _column_names;
    public:
        ColumnNameVisitor() = default;
        virtual ~ColumnNameVisitor() = default;

        virtual void visit(AndExpr *expr);
        virtual void visit(OrExpr *expr);
        virtual void visit(CompareExpr *expr);
        virtual void visit(RangeExpr *expr);
        virtual void visit(FalseExpr *expr);

        std::set<std::string> get() const;
    };
}

#endif //_DB_CONDITION_COLUMN_NAME_VISITOR_H_
