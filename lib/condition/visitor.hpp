#ifndef _DB_CONDITION_VISITOR_H_
#define _DB_CONDITION_VISITOR_H_

namespace cdb {
    struct ConsitionExpr;
    struct AndExpr;
    struct OrExpr;
    struct CompareExpr;

    class ConditionVisitor
    {
    public:
        virtual ~ConditionVisitor() = default;
        virtual void visit(AndExpr *) = 0;
        virtual void visit(OrExpr *) = 0;
        virtual void visit(CompareExpr *) = 0;
    };
}

#endif //_DB_CONDITION_VISITOR_H_
