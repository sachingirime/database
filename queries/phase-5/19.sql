SELECT SUM(l_discount) 
FROM customer, orders, lineitem
WHERE c_custkey = o_custkey AND o_orderkey = l_orderkey AND
	c_name = 'Customer#000000011' AND l_quantity > 25 AND l_discount < 0.03

