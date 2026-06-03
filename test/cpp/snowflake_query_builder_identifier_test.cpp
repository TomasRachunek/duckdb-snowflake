#include "snowflake_query_builder.hpp"

#include "duckdb/common/column_index.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/planner/filter/constant_filter.hpp"
#include "duckdb/planner/table_filter.hpp"

#include <iostream>
#include <memory>

namespace duckdb {
namespace snowflake {

static void AssertContains(const string &sql, const string &needle, const string &test_name) {
	if (!StringUtil::Contains(sql, needle)) {
		throw InternalException("%s failed: expected SQL to contain '%s', got '%s'", test_name.c_str(),
		                        needle.c_str(), sql.c_str());
	}
}

static void AssertEquals(const string &actual, const string &expected, const string &test_name) {
	if (actual != expected) {
		throw InternalException("%s failed: expected '%s', got '%s'", test_name.c_str(), expected.c_str(),
		                        actual.c_str());
	}
}

} // namespace snowflake
} // namespace duckdb

int main() {
	using namespace duckdb;
	using namespace duckdb::snowflake;

	try {
		AssertEquals(QuoteSnowflakeIdentifier("ORDERS"), "ORDERS", "uppercase identifier");
		AssertEquals(QuoteSnowflakeIdentifier("orders"), "\"orders\"", "lowercase identifier");
		AssertEquals(QuoteSnowflakeIdentifier("line item"), "\"line item\"", "spaced identifier");

		const auto projected_sql = SnowflakeQueryBuilder::BuildQuery(
		    "DATABASE.SCHEMA.ORDERS", {"orderId", "ORDER_TOTAL", "line item"}, nullptr, {"orderId", "ORDER_TOTAL"});
		AssertContains(projected_sql, "\"orderId\"", "projection quoting lowercase");
		AssertContains(projected_sql, "ORDER_TOTAL", "projection quoting uppercase");
		AssertContains(projected_sql, "\"line item\"", "projection quoting spaced");

		TableFilterSet filter_set;
		filter_set.PushFilter(ColumnIndex(0),
		                      std::make_unique<ConstantFilter>(ExpressionType::COMPARE_EQUAL, Value::INTEGER(42)));

		const auto filtered_sql =
		    SnowflakeQueryBuilder::BuildQuery("DATABASE.SCHEMA.ORDERS", {}, &filter_set, {"customerId"});
		AssertContains(filtered_sql, "\"customerId\"", "filter quoting lowercase");
		AssertContains(filtered_sql, "= 42", "filter equality");
	} catch (const std::exception &ex) {
		std::cerr << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
