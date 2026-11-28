Stage 2: the checker.

The purpose of this stage is to implement checkPlan() from PlanChecker. It determines if a query plan and its statisitcs and cost are correct. 

Review the changes made in stage 1 and the Excecution, OptTest, Catalog folders. 

11. A LogicalTableScan holds only its aliased input table. How is it matched to a (table, alias) pair of the FROM clause, and how is a scan of the un-aliased table, or a second scan of the same
alias, detected?

Compare a map of tables keyed by their alias with the original input table. Use the get functions in MyDB_LogicalOps and MyDB_Table. Review alias naming conventions to prepare for any expacted failure. 

Have a counter for each expected alias. If the counter is anything under than one, then something unexpected happend. 

12. How are two predicates compared? Are they compared using their pointers, or by their string
form (toString ())? Why does the choice matter for a plan built by someone else’s code?

Compare predicates with predicate->toString(). 

13. How is “exactly the attributes needed above it” computed for every op, top down, and what
does the root need?

Compute the root's attributes from SELECT and GROUP BY. Find the rest of the attributes by recursing through the map. See if the output aligns with what is expected. 

14. How are the statistics recomputed and compared, and with what tolerance for doubles?

Statistics are recomputed starting at the bottom. Use the costSelection() and costJoin() functions and the catalog to find the statistics while keeping the original order. For the doubles tolorance, keep values that are in 4 decimal places of the original value. 

15. What does the checker say about a plan that is correct but not optimal? For example, one that
applies a selection at a join instead of at the scan below it?

Accept correct plans even if they are not optimal. 

If a plan is incorrect, return a message with every failure the plan contains. Ensure the function does not crash from this. Return true if it passes all validation checks. 

Create 15 tests for stage 2. Tests should cover all aspects of the check correctness step. Wait for my approval before implementing any tests. Report all results, especially failures. 

Ask clarifying questions when needed. 