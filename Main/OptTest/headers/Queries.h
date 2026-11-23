#ifndef QUERIES_H
#define QUERIES_H

#include <string>
#include <vector>

// The queries in Examples/Examples.sql, as C++ strings, with the cost of the best plan for
// each (see that file for the comments).  Generated from Examples.sql: edit that file, not this.
//
//   for (auto &q : allQueries ()) { Optimized o (allTables, q.sql); ... q.cost ... }

struct ExampleQuery {
	std::string name;	// E1 ... E7, J3 ... J7, M1 ... M9
	std::string sql;
	double cost;		// of the best plan: the total number of tuples produced by its ops
};

inline std::vector <ExampleQuery> allQueries () {
	return {
		{"E1",
			"select l.l_comment from lineitem as l where (l.l_shipdate = \"1994-05-12\") and (l.l_commitdate = \"1994-05-22\") and (l.l_receiptdate = \"1994-06-10\")",
			1},
		{"E2",
			"select \"return flag was \" + l.l_returnflag, sum (l.l_quantity), sum (l.l_extendedprice), sum (l.l_extendedprice*(1-l.l_discount)), sum (l.l_extendedprice*(1-l.l_discount)*(1+l.l_tax)), avg (l.l_quantity), avg (l.l_extendedprice), avg (l.l_discount), sum (1) from lineitem as l where (l.l_shipdate < \"1998-12-01\") and (l.l_shipdate > \"1998-06-01\") group by l.l_returnflag",
			666801.666667},
		{"E3",
			"select l.l_shipmode, sum (1) from orders as o, lineitem as l where (o.o_orderkey = l.l_orderkey) and (o.o_orderstatus = \"F\") and (l.l_shipmode = \"TRUCK\" or l.l_shipmode = \"RAIL\") and (l.l_commitdate < l.l_receiptdate) and (l.l_shipdate < l.l_commitdate) and (l.l_receiptdate > \"1996-06-20\" or l.l_receiptdate = \"1996-06-20\") and (l.l_receiptdate < \"1997-06-20\" or l.l_receiptdate = \"1997-06-20\") group by l.l_shipmode",
			500073.050139},
		{"E4",
			"select ps.ps_partkey, avg (ps.ps_supplycost) from part as p, partsupp as ps, supplier as s where (p.p_partkey = ps.ps_partkey) and (s.s_suppkey = ps.ps_suppkey) group by ps.ps_partkey",
			2610256.08195},
		{"E5",
			"select ps.ps_partkey, avg (ps.ps_supplycost) from part as p, partsupp as ps, supplier as s, nation as n, region as r where (p.p_partkey = ps.ps_partkey) and (s.s_suppkey = ps.ps_suppkey) and (s.s_nationkey = n.n_nationkey) and (n.n_regionkey = r.r_regionkey) and (r.r_name = \"AMERICA\") group by ps.ps_partkey",
			1332082.21639},
		{"E6",
			"select n.n_name, o.o_orderdate, l.l_extendedprice * (1 - l.l_discount) - ps.ps_supplycost * l.l_quantity from part as p, supplier as s, lineitem as l, partsupp as ps, orders as o, nation as n where (s.s_suppkey = l.l_suppkey) and (ps.ps_suppkey = l.l_suppkey) and (ps.ps_partkey = l.l_partkey) and (p.p_partkey = l.l_partkey) and (o.o_orderkey = l.l_orderkey) and (s.s_nationkey = n.n_nationkey) and (p.p_type = \"STANDARD POLISHED TIN\" or p.p_type = \"ECONOMY BRUSHED TIN\" or p.p_type = \"ECONOMY POLISHED NICKEL\" or p.p_type = \"SMALL ANODIZED COPPER\")",
			8317377.22606},
		{"E7",
			"select \"both customers purchased part \" + p.p_name from part as p, orders as o1, orders as o2, lineitem as l1, lineitem as l2, customer as c1, customer as c2 where (l1.l_partkey = l2.l_partkey) and (o1.o_orderkey = l1.l_orderkey) and (o2.o_orderkey = l2.l_orderkey) and (o1.o_custkey = c1.c_custkey) and (o2.o_custkey = c2.c_custkey) and (c1.c_name = \"Customer#000129200\") and (c2.c_name = \"Customer#000108388\") and (p.p_partkey = l2.l_partkey)",
			15202520.9837},
		{"J3",
			"select l.l_shipmode, sum (1) from orders as o join lineitem as l on o.o_orderkey = l.l_orderkey where (o.o_orderstatus = \"F\") and (l.l_shipmode = \"TRUCK\" or l.l_shipmode = \"RAIL\") and (l.l_commitdate < l.l_receiptdate) and (l.l_shipdate < l.l_commitdate) and (l.l_receiptdate > \"1996-06-20\" or l.l_receiptdate = \"1996-06-20\") and (l.l_receiptdate < \"1997-06-20\" or l.l_receiptdate = \"1997-06-20\") group by l.l_shipmode",
			500073.050139},
		{"J4",
			"select ps.ps_partkey, avg (ps.ps_supplycost) from part as p join partsupp as ps on p.p_partkey = ps.ps_partkey join supplier as s on s.s_suppkey = ps.ps_suppkey group by ps.ps_partkey",
			2610256.08195},
		{"J5",
			"select ps.ps_partkey, avg (ps.ps_supplycost) from (part as p join partsupp as ps on p.p_partkey = ps.ps_partkey) join (supplier as s join (nation as n join region as r on n.n_regionkey = r.r_regionkey and r.r_name = \"AMERICA\") on s.s_nationkey = n.n_nationkey) on s.s_suppkey = ps.ps_suppkey group by ps.ps_partkey",
			1332082.21639},
		{"J6",
			"select n.n_name, o.o_orderdate, l.l_extendedprice * (1 - l.l_discount) - ps.ps_supplycost * l.l_quantity from part as p, supplier as s join nation as n on s.s_nationkey = n.n_nationkey, lineitem as l join partsupp as ps on ps.ps_suppkey = l.l_suppkey and ps.ps_partkey = l.l_partkey, orders as o where (s.s_suppkey = l.l_suppkey) and (p.p_partkey = l.l_partkey) and (o.o_orderkey = l.l_orderkey) and (p.p_type = \"STANDARD POLISHED TIN\" or p.p_type = \"ECONOMY BRUSHED TIN\" or p.p_type = \"ECONOMY POLISHED NICKEL\" or p.p_type = \"SMALL ANODIZED COPPER\")",
			8317377.22606},
		{"J7",
			"select \"both customers purchased part \" + p.p_name from (customer as c1 join orders as o1 on o1.o_custkey = c1.c_custkey and c1.c_name = \"Customer#000129200\") join lineitem as l1 on o1.o_orderkey = l1.l_orderkey join (lineitem as l2 join part as p on p.p_partkey = l2.l_partkey) on l1.l_partkey = l2.l_partkey join (orders as o2 join customer as c2 on o2.o_custkey = c2.c_custkey and c2.c_name = \"Customer#000108388\") on o2.o_orderkey = l2.l_orderkey",
			15202520.9837},
		{"M1",
			"select n.n_name, r.r_name from nation as n, region as r",
			155},
		{"M2",
			"select n.n_name, r.r_name from nation as n, region as r where r.r_name = \"AMERICA\"",
			51},
		{"M3",
			"select n.n_name, r.r_name from nation as n, region as r where (n.n_regionkey = r.r_regionkey) and (n.n_name = \"FRANCE\" or r.r_name = \"AMERICA\")",
			35},
		{"M4",
			"select s.s_name from supplier as s, nation as n where (s.s_nationkey = n.n_nationkey) and (n.n_name = \"FRANCE\")",
			10401},
		{"M5",
			"select c1.c_name, c2.c_name from customer as c1, customer as c2 where (c1.c_nationkey = c2.c_nationkey) and (c1.c_name = \"Customer#000000001\")",
			155929.529406},
		{"M6",
			"select r.r_name from region as r",
			5},
		{"M7",
			"select l.l_orderkey from lineitem as l where (not l.l_quantity < 10) and (l.l_extendedprice * (1 - l.l_discount) > 50000)",
			2000405},
		{"M8",
			"select p.p_name, n.n_name from part as p, partsupp as ps, supplier as s, nation as n where (p.p_partkey = ps.ps_partkey) and (ps.ps_suppkey = s.s_suppkey) and (s.s_nationkey = n.n_nationkey) and (p.p_size = 15) and (n.n_name = \"FRANCE\")",
			831046.326504},
		{"M9",
			"select p.p_name, n.n_name from (part as p join partsupp as ps on p.p_partkey = ps.ps_partkey and p.p_size = 15) join (supplier as s join nation as n on s.s_nationkey = n.n_nationkey and n.n_name = \"FRANCE\") on ps.ps_suppkey = s.s_suppkey",
			831046.326504},
	};
}

inline ExampleQuery query (std::string name) {
	for (auto &q : allQueries ())
		if (q.name == name)
			return q;
	return {"", "", -1};
}

#endif
