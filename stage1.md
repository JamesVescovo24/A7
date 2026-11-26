Stage 1: the mapping.
1. T : what C++ value holds the set of tables being optimized, is it keyed by table name or by
alias, and why does the difference matter? 

The tables are stored as vector<pair<string, string>> tablesToProccess, the first string is the table name and the second is the table's alias. The tables should all be keyed according to their alias. Alias's can distinguish between queries to the same table. 

2. C: what holds the CNF, where does the top-level CNF come from, and why must the relative
order of its disjunctions be preserved every time it is split into leftCNF, rightCNF and topCNF ?

CNF is held by vector<ExprTreePtr>. The top-level CNF comes from vector<ExprTreePtr> predicates = getAllPredicates(). Order matters when it is called in MyDB_Stats.

3. “Refers only to the left”: exactly how is it decided for one disjunction, with which method, and
where does a disjunction go that mentions no table at all (for example: WHERE 1 < 2)? It has
to go somewhere consistently (Stage 3 depends on this).

Find the predicate inside the table references. For each disjunction, iterate through the alias and see if it references either side. If only reference left -> leftCNF. If only reference right -> rightCNF. Else disjunction -> topCNF. If no table is specified send it to the smallest table alias stored. 

4. A: what holds the set of attributes needed, what are their names, where does the top-level A
come from, and how are leftAtts and rightAtts computed from A and topCNF ?

The set of attributes is stored in MyDB_SchemaPtr totSchema. The attribute names are formatted as [alias]_[attributeName]. Top level A comes from valuesToSelect (SELECT) and groupingClauses (GROUP BY). 

leftAtts = atts (left) intersect (A union atts (topCNF))
rightAtts = atts (right) intersect (A union atts (topCNF))

5. getStats: How are methods on MyDB_Stats called, with exactly which arguments, for a scan
and for a join?

For scans construct MyDB_Stats from alias input table and call costSelection() with the current CNF. For joins, call costJoin() on the left child, passing topCNF and right child's. 

6. allSubsets: how are the splits enumerated?

T=set, T'=subset. For each T' that is not empty and doesn't span all of T, the left split equals T and the right split equals T -T'. Create new maps for each splitted subset, recurse on both sides. 

7. The LogicalOps: what goes into each argument of the LogicalTableScan and LogicalJoin
constructors, and how are the output tables named?

LogicalTableScan: MyDB_TablePtr input, MyDB_TablePtr output, MyDB_StatsPtr stats, disjunctions
LogicalJoin: left, right, top, outputTable, joinStats 
output tables are named based on the aliases of the joined tables. 

8. The return value: what pair comes back, and how is the cost of the whole plan accumulated?
What does the top-level optimizeQueryPlan (allTables) do before the recursion starts?

They return pair<LogicalOpPtr, double>. Calculate best cost by calculating every candidate cost and keeping the one with the lowest. 

9. Edge cases: How to handle the following? A query with no WHERE clause; two tables with no
predicate linking them; a table that appears in the FROM and WHERE clauses but not in SELECT
or GROUP BY; the same table under two aliases.

If no WHERE or ON, return an empty vector. The optimizer shouldn't skip a split because it's missing a predicate. If a table that appears in the FROM and WHERE clauses but not in SELECT
or GROUP BY have it's output schema be empty. If the same table under two aliases, so long as the alias' are different there should be no overlap. 

10. Where is numCalls incremented?

At the start if every recursive optimizer call.

The purpose of this stage is to implement an exhaustive enumeration algorithm in order tp find the query with the lowest cost by recursively splitting the input tables. Each subset of tables would be optimized and calculate the cost of joining them together. Build the optimizeQueryPlan() in SFWQuery, look at any files it references before coding.  

1. The tables are stored as vector<pair<string, string>> tablesToProccess, the first string is the table name and the second is the table's alias. The tables are keyed to their orignal name inside of allTables. Create a new map keying the tables by their aliases. The aliases are used to distinguish queries to the same tables.

2. CNF is held by vector<ExprTreePtr>, each element is a disjunction and theyare all joined together. The top-level CNF comes from vector<ExprTreePtr> predicates = getAllPredicates(). For each recursive split, create a leftCNF, rightCNF, and topCNF. Iterate through the current CNF in order to add each disjunction to the correct CNF. Order matters when it is called in MyDB_Stats.

3. Iterate through the alias in the left and right tables for every disjunction. Use the alias to see if it references either side. Find the predicate inside the table references. If only reference left -> leftCNF. If only reference right -> rightCNF. if references both -> topCNF. If no table is specified send it to the smallest table alias stored. 

4. The set of attributes is stored in MyDB_SchemaPtr totSchema. The attribute names are formatted as [alias]_[attributeName]. Top level A comes from valuesToSelect (SELECT) and groupingClauses (GROUP BY). Create a recursive function to find the attributes referenced, use the following calculations: 

leftAtts = atts (left) intersect (A union atts (topCNF))
rightAtts = atts (right) intersect (A union atts (topCNF))

Use the alias and attribute order when creating schemas. 

5. For scans construct MyDB_Stats from alias input table and call costSelection() with the current CNF. For joins, call costJoin() on the left child, passing topCNF and right child's statistics. costSelection() and costJoin() should be ordered by predicates. The output handles projection. 

6. T represents the set of tables and T' is a subset of T.  For each T' that is not empty and doesn't span all of T, the left split equals T' and the right split equals T - T'. Create a new map <string, MyDB_TablePtr> for each side, with the predicates moved into leftCNF, rightCNF, and topCNF. Recursively call on optimizeQueryPlan(). 

7. Use MyDB_Table to create the output table specifications. Name the outputs based on the aliases, ensure there is no change for collisions. 

LogicalTableScan: MyDB_TablePtr input, MyDB_TablePtr output, MyDB_StatsPtr stats, disjunctions
LogicalJoin: left, right, top, outputTable, joinStats 
output tables are named based on the aliases of the joined tables.

8. optimizeQueryPlan returns pair<LogicalOpPtr, double>. The first is the cheapest plan, the second is the estimated cost. Calculate best cost by calculating every candidate cost and keeping the one with the lowest. 

9. If no WHERE or ON, return an empty vector. The optimizer shouldn't skip a split because it's missing a predicate. If a table that appears in the FROM and WHERE clauses but not in SELECT
or GROUP BY have it's output schema be empty. If the same table under two aliases, so long as the alias' are different there should be no overlap. 

10. Increment numCalls once in the beginning of every call to the recursive optimizeQueryPlan. Do not increment it in the top level optimizer or sepreatly during splits. 

Do not implement the plan checker or memorization in stage 1. Before writing any code, review all relevant files for this prompt and the already exisitng tests such as in StudentQUnitStage1.cc, OptQUnit.cc, Queries.h, TestHelpers.h. Check the optimizer works by comparing it to Examples.sql. Tests should cover all aspects of the optimizer step. Wait for my approval before implementing any tests. Report all results, especially failures. 

Ask clarifying questions when needed. 