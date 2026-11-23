-- A7 example queries, over the TPC-H catalog in Build/bin/myCat.  Run them with
--
--     cd Build/bin
--     ./sqlUnitTest myCat . < ../../Examples/Examples.sql
--
-- The line before each query gives the cost (the total number of tuples produced by every
-- operation in the plan) of the best plan, which is what a correct optimizer prints as "Cost:".
-- Queries E1 to E7 use the comma-separated FROM clause; J3 to J7 are the same queries written
-- with JOIN ... ON, and must get the same cost.  M1 to M9 are more cases.

-- E1.  one table, selections only
-- cost: 1
select
        l.l_comment
from
        lineitem as l
where
        (l.l_shipdate = "1994-05-12") and
        (l.l_commitdate = "1994-05-22") and
        (l.l_receiptdate = "1994-06-10");

-- E2.  one table, with aggregates and a GROUP BY
-- cost: 666802
select
        "return flag was " + l.l_returnflag,
        sum (l.l_quantity),
        sum (l.l_extendedprice),
        sum (l.l_extendedprice*(1-l.l_discount)),
        sum (l.l_extendedprice*(1-l.l_discount)*(1+l.l_tax)),
        avg (l.l_quantity),
        avg (l.l_extendedprice),
        avg (l.l_discount),
        sum (1)
from
        lineitem as l
where
        (l.l_shipdate < "1998-12-01") and
        (l.l_shipdate > "1998-06-01")
group by
        l.l_returnflag;

-- E3.  two tables
-- cost: 500073
select
        l.l_shipmode,
        sum (1)
from
        orders as o,
        lineitem as l
where
        (o.o_orderkey = l.l_orderkey)
        and (o.o_orderstatus = "F")
        and (l.l_shipmode = "TRUCK" or l.l_shipmode = "RAIL")
        and (l.l_commitdate < l.l_receiptdate)
        and (l.l_shipdate < l.l_commitdate)
        and (l.l_receiptdate > "1996-06-20" or l.l_receiptdate = "1996-06-20")
        and (l.l_receiptdate < "1997-06-20" or l.l_receiptdate = "1997-06-20")
group by
        l.l_shipmode;

-- E4.  three tables
-- cost: 2.61026e+06
select
        ps.ps_partkey,
        avg (ps.ps_supplycost)
from
        part as p,
        partsupp as ps,
        supplier as s
where
        (p.p_partkey = ps.ps_partkey)
        and (s.s_suppkey = ps.ps_suppkey)
group by
        ps.ps_partkey;

-- E5.  five tables
-- cost: 1.33208e+06
select
        ps.ps_partkey,
        avg (ps.ps_supplycost)
from
        part as p,
        partsupp as ps,
        supplier as s,
        nation as n,
        region as r
where
        (p.p_partkey = ps.ps_partkey)
        and (s.s_suppkey = ps.ps_suppkey)
        and (s.s_nationkey = n.n_nationkey)
        and (n.n_regionkey = r.r_regionkey)
        and (r.r_name = "AMERICA")
group by
        ps.ps_partkey;

-- E6.  six tables
-- cost: 8.31738e+06
select
        n.n_name,
        o.o_orderdate,
        l.l_extendedprice * (1 - l.l_discount) - ps.ps_supplycost * l.l_quantity
from
        part as p,
        supplier as s,
        lineitem as l,
        partsupp as ps,
        orders as o,
        nation as n
where
        (s.s_suppkey = l.l_suppkey)
        and (ps.ps_suppkey = l.l_suppkey)
        and (ps.ps_partkey = l.l_partkey)
        and (p.p_partkey = l.l_partkey)
        and (o.o_orderkey = l.l_orderkey)
        and (s.s_nationkey = n.n_nationkey)
        and (p.p_type = "STANDARD POLISHED TIN" or
             p.p_type = "ECONOMY BRUSHED TIN" or
             p.p_type = "ECONOMY POLISHED NICKEL" or
             p.p_type = "SMALL ANODIZED COPPER");

-- E7.  seven tables, two of them used twice.  Without memoization this one takes a while
-- cost: 1.52025e+07
select
        "both customers purchased part " + p.p_name
from
        part as p,
        orders as o1,
        orders as o2,
        lineitem as l1,
        lineitem as l2,
        customer as c1,
        customer as c2
where
        (l1.l_partkey = l2.l_partkey)
        and (o1.o_orderkey = l1.l_orderkey)
        and (o2.o_orderkey = l2.l_orderkey)
        and (o1.o_custkey = c1.c_custkey)
        and (o2.o_custkey = c2.c_custkey)
        and (c1.c_name = "Customer#000129200")
        and (c2.c_name = "Customer#000108388")
        and (p.p_partkey = l2.l_partkey);

-- J3.  E3 with JOIN ... ON.  Same cost as E3
-- cost: 500073
select
        l.l_shipmode,
        sum (1)
from
        orders as o join lineitem as l on o.o_orderkey = l.l_orderkey
where
        (o.o_orderstatus = "F")
        and (l.l_shipmode = "TRUCK" or l.l_shipmode = "RAIL")
        and (l.l_commitdate < l.l_receiptdate)
        and (l.l_shipdate < l.l_commitdate)
        and (l.l_receiptdate > "1996-06-20" or l.l_receiptdate = "1996-06-20")
        and (l.l_receiptdate < "1997-06-20" or l.l_receiptdate = "1997-06-20")
group by
        l.l_shipmode;

-- J4.  E4 with JOIN ... ON, and no WHERE clause at all.  Same cost as E4
-- cost: 2.61026e+06
select
        ps.ps_partkey,
        avg (ps.ps_supplycost)
from
        part as p join partsupp as ps on p.p_partkey = ps.ps_partkey
        join supplier as s on s.s_suppkey = ps.ps_suppkey
group by
        ps.ps_partkey;

-- J5.  E5 with the joins grouped by parentheses, and the selection in an ON.  Same cost as E5
-- cost: 1.33208e+06
select
        ps.ps_partkey,
        avg (ps.ps_supplycost)
from
        (part as p join partsupp as ps on p.p_partkey = ps.ps_partkey)
        join (supplier as s join (nation as n join region as r
                on n.n_regionkey = r.r_regionkey and r.r_name = "AMERICA")
              on s.s_nationkey = n.n_nationkey)
        on s.s_suppkey = ps.ps_suppkey
group by
        ps.ps_partkey;

-- J6.  E6 with a mix of JOIN ... ON and commas.  Same cost as E6
-- cost: 8.31738e+06
select
        n.n_name,
        o.o_orderdate,
        l.l_extendedprice * (1 - l.l_discount) - ps.ps_supplycost * l.l_quantity
from
        part as p,
        supplier as s join nation as n on s.s_nationkey = n.n_nationkey,
        lineitem as l join partsupp as ps on ps.ps_suppkey = l.l_suppkey and ps.ps_partkey = l.l_partkey,
        orders as o
where
        (s.s_suppkey = l.l_suppkey)
        and (p.p_partkey = l.l_partkey)
        and (o.o_orderkey = l.l_orderkey)
        and (p.p_type = "STANDARD POLISHED TIN" or
             p.p_type = "ECONOMY BRUSHED TIN" or
             p.p_type = "ECONOMY POLISHED NICKEL" or
             p.p_type = "SMALL ANODIZED COPPER");

-- J7.  E7 with JOIN ... ON.  Same cost as E7
-- cost: 1.52025e+07
select
        "both customers purchased part " + p.p_name
from
        (customer as c1 join orders as o1 on o1.o_custkey = c1.c_custkey and c1.c_name = "Customer#000129200")
        join lineitem as l1 on o1.o_orderkey = l1.l_orderkey
        join (lineitem as l2 join part as p on p.p_partkey = l2.l_partkey) on l1.l_partkey = l2.l_partkey
        join (orders as o2 join customer as c2 on o2.o_custkey = c2.c_custkey and c2.c_name = "Customer#000108388")
        on o2.o_orderkey = l2.l_orderkey;

-- M1.  no WHERE clause and no join predicate: a cross product
-- cost: 155
select
        n.n_name, r.r_name
from
        nation as n, region as r;

-- M2.  two tables with a selection but no join predicate: a cross product of the filtered inputs
-- cost: 51
select
        n.n_name, r.r_name
from
        nation as n, region as r
where
        r.r_name = "AMERICA";

-- M3.  a disjunction that mentions two tables cannot be pushed down; it stays at the join
-- cost: 35
select
        n.n_name, r.r_name
from
        nation as n, region as r
where
        (n.n_regionkey = r.r_regionkey)
        and (n.n_name = "FRANCE" or r.r_name = "AMERICA");

-- M4.  a table that appears only in the WHERE clause (never in SELECT or GROUP BY): its scan
-- needs only the attributes the predicates use
-- cost: 10401
select
        s.s_name
from
        supplier as s, nation as n
where
        (s.s_nationkey = n.n_nationkey)
        and (n.n_name = "FRANCE");

-- M5.  a self join: the same table under two aliases
-- cost: 155930
select
        c1.c_name, c2.c_name
from
        customer as c1, customer as c2
where
        (c1.c_nationkey = c2.c_nationkey)
        and (c1.c_name = "Customer#000000001");

-- M6.  a single table with no WHERE clause
-- cost: 5
select
        r.r_name
from
        region as r;

-- M7.  a predicate that is not a comparison: NOT, and a comparison with an expression
-- cost: 2.0004e+06
select
        l.l_orderkey
from
        lineitem as l
where
        (not l.l_quantity < 10)
        and (l.l_extendedprice * (1 - l.l_discount) > 50000);

-- M8.  a four-table chain where the best plan is bushy, not left-deep:
--      (part JOIN partsupp) JOIN (supplier JOIN nation)
-- cost: 831046
select
        p.p_name, n.n_name
from
        part as p, partsupp as ps, supplier as s, nation as n
where
        (p.p_partkey = ps.ps_partkey)
        and (ps.ps_suppkey = s.s_suppkey)
        and (s.s_nationkey = n.n_nationkey)
        and (p.p_size = 15)
        and (n.n_name = "FRANCE");

-- M9.  the same, written with JOIN ... ON
-- cost: 831046
select
        p.p_name, n.n_name
from
        (part as p join partsupp as ps on p.p_partkey = ps.ps_partkey and p.p_size = 15)
        join (supplier as s join nation as n on s.s_nationkey = n.n_nationkey and n.n_name = "FRANCE")
        on ps.ps_suppkey = s.s_suppkey;
