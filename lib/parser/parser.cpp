#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <stack>

#include "lib/condition/condition.hpp"
#include "lib/table/schema.hpp"
#include "third-party/pegtl/pegtl.hh"
#include "third-party/pegtl/pegtl/trace.hh"
#include "parser.hpp"
#include "lib/condition/dump-visitor.hpp"

using namespace cdb;

namespace cdb {
    template <typename T>
    struct token
        : pegtl::seq<
            T,
            pegtl::star<pegtl::space>
          >
    { };

    template <typename T>
    struct paren
        : pegtl::seq<
            token<pegtl::one<'('> >,
            T,
            token<pegtl::one<')'> >
          >
    { };

    template <typename T>
    struct stmt
        : pegtl::seq<
            T,
            token<pegtl::one<';'> >
          >
    { };

    struct integer
        : pegtl::plus<pegtl::digit>
    { };

    struct float_
        : pegtl::seq<
            integer,
            pegtl::opt<pegtl::seq<
                pegtl::one<'.'>,
                integer
            > >
          >
    { };

    struct string
        : pegtl::if_must<
            pegtl::one<'\''>,
            pegtl::star<pegtl::not_one<'\''>>,
            pegtl::one<'\''>
          >
    { };

    struct table_name
        : token<pegtl::identifier >
    { };

    struct index_name
        : token<pegtl::identifier >
    { };

    struct field_name
        : token<pegtl::identifier >
    { };

    struct int_type
        : token<pegtl_istring_t("int") >
    { };

    struct float_type
        : token<pegtl_istring_t("float") >
    { };

    struct char_type_length
        : token<integer >
    { };

    struct char_type
        : pegtl::seq<
            token<pegtl_istring_t("char") >,
            paren<char_type_length >
          >
    { };

    struct value
        : pegtl::sor<
            token<string>,
            token<float_>,
            token<integer>
          >
    { };

    struct field_type
        : pegtl::sor<
            int_type,
            float_type,
            char_type
          >
    { };

    struct column_attr
        : pegtl::sor<
            token<pegtl_istring_t("unique") >,
            token<pegtl_istring_t("auto increment") >
          >
    { };

    struct column_decl
        : pegtl::seq<
            field_name,
            field_type,
            pegtl::star<column_attr>
          >
    { };

    struct column_primary_decl
        : pegtl::seq<
            token<pegtl_istring_t("primary")>,
            token<pegtl_istring_t("key")>,
            paren<field_name>
          >
    { };

    struct column_decl_list
        : pegtl::list<
            pegtl::sor<
                column_decl,
                column_primary_decl
            >,
            token<pegtl::one<','> >
          >
    { };

    struct create_table_stmt
        : stmt<pegtl::seq<
            token<pegtl_istring_t("create") >,
            token<pegtl_istring_t("table") >,
            table_name,
            paren<column_decl_list >
          > >
    { };

    struct drop_table_stmt
        : stmt<pegtl::seq<
            token<pegtl_istring_t("drop") >,
            token<pegtl_istring_t("table") >,
            table_name
          > >
    { };

    struct create_index_stmt
        : stmt<pegtl::seq<
            token<pegtl_istring_t("create") >,
            token<pegtl_istring_t("index") >,
            index_name,
            token<pegtl_istring_t("on") >,
            table_name,
            paren<field_name >
          > >
    { };

    struct drop_index_stmt
        : stmt<pegtl::seq<
            token<pegtl_istring_t("drop") >,
            token<pegtl_istring_t("index") >,
            index_name
          > >
    { };

    struct select_column_name
        : pegtl::must<field_name>
    { };

    struct column_list
        : pegtl::list<
            select_column_name,
            token<pegtl::one<','> >
          >
    { };

    struct column_set
        : pegtl::sor<
            token<pegtl::one<'*'> >,
            column_list
          >
    { };

    struct condition_compare_op
        : pegtl::sor<
            pegtl_istring_t("="),
            pegtl_istring_t("<>"),
            pegtl_istring_t(">"),
            pegtl_istring_t(">="),
            pegtl_istring_t("<"),
            pegtl_istring_t("<=")
          >
    { };

    struct condition_compare
        : pegtl::seq<
            field_name,
            token<condition_compare_op>,
            value
          >
    { };

    struct condition_or;

    struct condition_subexpr
        : paren<condition_or>
    { };

    struct condition_and
        : pegtl::seq<
            pegtl::sor<
                condition_subexpr,
                condition_compare
            >,
            pegtl::opt<
                token<pegtl_istring_t("and") >,
                condition_and
            >
        >
    { };

    struct condition_or
        : pegtl::seq<
            condition_and,
            pegtl::opt<
                token<pegtl_istring_t("or") >,
                condition_or
            >
        >
    { };

    struct select_stmt
        : stmt<pegtl::seq<
            token<pegtl_istring_t("select")>,
            column_set,
            token<pegtl_istring_t("from")>,
            table_name,
            pegtl::opt<pegtl::seq<
                token<pegtl_istring_t("where")>,
                condition_or
            > >
          > >
    { };

    struct insert_value
        : pegtl::must<value>
    { };

    struct value_set
        : paren<
            pegtl::list<
                insert_value,
                token<pegtl::one<','> >
            >
          >
    { };

    struct insert_stmt
        : stmt<pegtl::seq<
            token<pegtl_istring_t("insert")>,
            token<pegtl_istring_t("into")>,
            table_name,
            token<pegtl_istring_t("values")>,
            pegtl::list<
                value_set,
                token<pegtl::one<','> >
            >
          > >
    { };

    struct delete_stmt
        : stmt<pegtl::seq<
            token<pegtl_istring_t("delete")>,
            token<pegtl_istring_t("from")>,
            table_name,
            pegtl::opt<pegtl::seq<
                token<pegtl_istring_t("where")>,
                condition_or
            > >
          > >
    { };

    struct quit_stmt
        : stmt<token<pegtl_istring_t("quit") > >
    { };

    struct exec_stmt
        : stmt<pegtl::seq<
            token<pegtl_istring_t("execfile") >,
            token<string>
          > >
    { };

    struct statment
        : pegtl::seq<
              pegtl::star<pegtl::space >,
              pegtl::sor<
                create_table_stmt,
                drop_table_stmt,
                create_index_stmt,
                drop_index_stmt,
                insert_stmt,
                select_stmt,
                delete_stmt,
                quit_stmt,
                exec_stmt
              >
          >
    { };

    struct stmt_list
        : pegtl::star<
            statment
          >
    { };

    struct ParseState
    {
        enum class LastMatchCondition
        {
            COMPARE,
            AND,
            OR
        };

        std::string integer;
        std::string float_;
        std::string string_;
        std::string id;
        std::string table_name;
        std::string index_name;
        std::string field_name;
        std::string char_type;
        std::string field_type;
        std::string compare_op;
        std::string value;

        LastMatchCondition last_matched = LastMatchCondition::COMPARE;

        std::unique_ptr<Schema::Factory> schema_builder;
        std::unique_ptr<Table::RecordBuilder> record_builder;
        std::stack<std::unique_ptr<ConditionExpr> > condition_expr;
        std::vector<std::string> column_list;
        std::vector<std::string> insert_value_set;

        Database *db;
        Parser *parser;
    };

    template <typename Rule>
    struct ParseAction
    {
        static void
        apply(const pegtl::input &, ParseState &)
        { }
    };

    template <>
    struct ParseAction<integer>
    {
        static void
        apply(const pegtl::input &in, ParseState &state)
        {
            state.value = state.integer = in.string();
        }
    };

    template <>
    struct ParseAction<float_>
    {
        static void
        apply(const pegtl::input &in, ParseState &state)
        { 
            state.value = state.float_ = in.string();
        }
    };

    template <>
    struct ParseAction<string>
    {
        static void
        apply(const pegtl::input &in, ParseState &state)
        { 
            state.string_ = in.string();
            state.value = state.string_ = state.string_.substr(1, state.string_.length() - 2);
        }
    };

    template <>
    struct ParseAction<pegtl::identifier>
    {
        static void
        apply(const pegtl::input &in, ParseState &state)
        { state.id = in.string(); }
    };

    template <>
    struct ParseAction<table_name>
    {
        static void
        apply(const pegtl::input &, ParseState &state)
        { 
            state.table_name = state.id;
        }
    };

    template <>
    struct ParseAction<index_name>
    {
        static void
        apply(const pegtl::input &, ParseState &state)
        { 
            state.index_name = state.id;
        }
    };

    template <>
    struct ParseAction<field_name>
    {
        static void
        apply(const pegtl::input &, ParseState &state)
        {
            state.field_name = state.id;
        }
    };

    template <>
    struct ParseAction<int_type>
    {
        static void
        apply(const pegtl::input &, ParseState &state)
        { 
            state.field_type = "int";
        }
    };

    template <>
    struct ParseAction<float_type>
    {
        static void
        apply(const pegtl::input &, ParseState &state)
        { 
            state.field_type = "float";
        }
    };

    template <>
    struct ParseAction<char_type_length>
    {
        static void
        apply(const pegtl::input &in, ParseState &state)
        { 
            state.field_type = std::to_string(std::stoi(in.string()) + 1);
        }
    };

    template <>
    struct ParseAction<column_decl>
    {
        static void
        apply(const pegtl::input &, ParseState &state)
        {
            if (!state.schema_builder) {
                state.schema_builder.reset(new Schema::Factory());
            }

            if (state.field_type == "int") {
                state.schema_builder->addIntegerField(state.field_name);
            }
            else if (state.field_type == "float") {
                state.schema_builder->addFloatField(state.field_name);
            }
            else {
                state.schema_builder->addCharField(state.field_name, std::stoi(state.field_type));
            }
        }
    };

    template <>
    struct ParseAction<column_primary_decl>
    {
        static void
        apply(const pegtl::input &, ParseState &state)
        {
            if (!state.schema_builder) {
                throw SchemaColumnNotFoundException(state.field_name);
            }
            state.schema_builder->setPrimary(state.field_name);
        }
    };

    template <>
    struct ParseAction<create_table_stmt>
    {
        static void
        apply(const pegtl::input &, ParseState &state)
        {
            std::unique_ptr<Schema> schema(state.schema_builder->release());
            state.db->createTable(
                    state.table_name,
                    schema.get()
                );
            state.db->updateRootTable();
        }
    };

    template <>
    struct ParseAction<drop_table_stmt>
    {
        static void
        apply(const pegtl::input &, ParseState &state)
        {
            state.db->dropTable(state.table_name);
            state.db->updateRootTable();
        }
    };

    template <>
    struct ParseAction<create_index_stmt>
    {
        static void
        apply(const pegtl::input &, ParseState &state)
        {
            auto *table = state.db->getTableByName(state.table_name);
            table->createIndex(state.field_name, state.index_name);
            state.db->updateRootTable();
        }
    };

    template <>
    struct ParseAction<drop_index_stmt>
    {
        static void
        apply(const pegtl::input &, ParseState &state)
        {
            auto table_name = state.db->indexFor(state.index_name);
            auto *table = state.db->getTableByName(table_name);
            table->dropIndex(state.index_name);
            state.db->updateRootTable();
        }
    };

    template <>
    struct ParseAction<select_column_name>
    {
        static void
        apply(const pegtl::input &, ParseState &state)
        { state.column_list.push_back(state.field_name); }
    };

    template <>
    struct ParseAction<condition_compare_op>
    {
        static void
        apply(const pegtl::input &in, ParseState &state)
        { state.compare_op = in.string(); }
    };

    template <>
    struct ParseAction<condition_compare>
    {
        static void
        apply(const pegtl::input &, ParseState &state)
        {
            CompareExpr::Operator op;

            if (state.compare_op == "=") {
                op = CompareExpr::Operator::EQ;
            }
            else if (state.compare_op == "<>") {
                op = CompareExpr::Operator::NE;
            }
            else if (state.compare_op == ">=") {
                op = CompareExpr::Operator::GE;
            }
            else if (state.compare_op == "<=") {
                op = CompareExpr::Operator::LE;
            }
            else if (state.compare_op == ">") {
                op = CompareExpr::Operator::GT;
            }
            else {
                op = CompareExpr::Operator::LT;
            }

            state.condition_expr.emplace(
                    new CompareExpr(
                        state.field_name,
                        op,
                        state.value
                    )
                );

            state.last_matched = ParseState::LastMatchCondition::COMPARE;
        }
    };

    template <>
    struct ParseAction<condition_and>
    {
        static void
        apply(const pegtl::input &, ParseState &state)
        {
            if (state.last_matched == ParseState::LastMatchCondition::COMPARE) {
                state.last_matched = ParseState::LastMatchCondition::AND;
                return;
            }

            auto a = state.condition_expr.top().release(); state.condition_expr.pop();
            auto b = state.condition_expr.top().release(); state.condition_expr.pop();

            state.condition_expr.emplace(
                    new AndExpr(a, b)
                );
        }
    };

    template <>
    struct ParseAction<condition_or>
    {
        static void
        apply(const pegtl::input &, ParseState &state)
        {
            if (state.last_matched == ParseState::LastMatchCondition::AND) {
                state.last_matched = ParseState::LastMatchCondition::OR;
                return;
            }
            auto a = state.condition_expr.top().release(); state.condition_expr.pop();
            auto b = state.condition_expr.top().release(); state.condition_expr.pop();

            state.condition_expr.emplace(
                    new OrExpr(a, b)
                );
        }
    };

    template <>
    struct ParseAction<select_stmt>
    {
        static void
        apply(const pegtl::input &, ParseState &state)
        {
            auto *table = state.db->getTableByName(state.table_name);
            std::unique_ptr<Schema> schema;
            if (state.column_list.size()) {
                schema.reset(table->buildSchemaFromColumnNames(state.column_list));
                state.column_list.clear();
            }
            else {
                schema.reset(table->getSchema()->copy());
            }
            std::unique_ptr<ConditionExpr> condition(
                    state.condition_expr.size() ? 
                        state.condition_expr.top().release() :
                        nullptr
                );
            if (condition) {
                state.condition_expr.pop();
                condition.reset(table->optimizeCondition(condition.release()));
            }

            for (auto &field : *schema) {
                std::cout << field.name << "\t";
            }
            std::cout << std::endl;

            table->select(
                    schema.get(),
                    condition.get(),
                    [&](const ConstSlice &row)
                    {
                        auto length = schema->end() - schema->begin();
                        for (int i = 0 ; i < length; ++i) {
                            auto col = schema->getColumnById(i);
                            std::cout << Convert::toString(
                                    col.getType(),
                                    col.getValue(row)
                                );
                            std::cout << "\t";
                        }
                        std::cout << std::endl;
                    }
                );
        }
    };

    template <>
    struct ParseAction<pegtl_istring_t("values") >
    {
        static void
        apply(const pegtl::input &, ParseState &state)
        {
            auto *table = state.db->getTableByName(state.table_name);
            state.record_builder.reset(table->getRecordBuilder());
        }
    };

    template <>
    struct ParseAction<insert_value>
    {
        static void
        apply(const pegtl::input &, ParseState &state)
        { state.insert_value_set.push_back(state.value); }
    };

    template <>
    struct ParseAction<value_set>
    {
        static void
        apply(const pegtl::input &, ParseState &state)
        {
            state.record_builder->addRow();
            for (auto &value : state.insert_value_set) {
                state.record_builder->addValue(value);
            }
            state.insert_value_set.clear();
        }
    };

    template <>
    struct ParseAction<insert_stmt>
    {
        static void
        apply(const pegtl::input &, ParseState &state)
        {
            auto *table = state.db->getTableByName(state.table_name);
            table->insert(state.record_builder->getSchema(), state.record_builder->getRows());
        }
    };

    template <>
    struct ParseAction<delete_stmt>
    {
        static void
        apply(const pegtl::input &, ParseState &state)
        {
            auto *table = state.db->getTableByName(state.table_name);
            std::unique_ptr<ConditionExpr> condition(
                    state.condition_expr.size() ? 
                        state.condition_expr.top().release() :
                        nullptr
                );
            if (condition) {
                state.condition_expr.pop();
                condition.reset(table->optimizeCondition(condition.release()));
            }

            table->erase(condition.get());
        }
    };

    template <>
    struct ParseAction<quit_stmt>
    {
        static void
        apply(const pegtl::input &, ParseState &)
        {
            throw ParserQuitingException();
        }
    };

    template <>
    struct ParseAction<exec_stmt>
    {
        static void
        apply(const pegtl::input &, ParseState &state)
        {
            state.parser->exec_file(state.string_);
        }
    };
}

void
Parser::exec(std::string sql)
{
    ParseState state;
    state.db = _db;
    state.parser = this;
    auto result = pegtl::parse<stmt_list, ParseAction>(sql, "__TERMINAL__", state);
    if (!result) {
        throw ParserSyntaxErrorException();
    }
}

void
Parser::exec_file(std::string name)
{
    ParseState state;
    state.db = _db;
    state.parser = this;

    try {
        pegtl::file_parser file_parser(name);
        ParseState new_state;
        new_state.db = state.db;
        auto result = file_parser.parse<stmt_list, ParseAction>(new_state);
        if (!result) {
            throw ParserSyntaxErrorException();
        }
    }
    catch (ParserQuitingException) {};
}
