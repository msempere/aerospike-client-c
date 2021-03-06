/*
 * Copyright 2008-2014 Aerospike, Inc.
 *
 * Portions may be licensed to Aerospike, Inc. under one or more contributor
 * license agreements.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */
#include <aerospike/aerospike.h>
#include <aerospike/aerospike_key.h>

#include <aerospike/as_error.h>
#include <aerospike/as_status.h>

#include <aerospike/as_record.h>
#include <aerospike/as_integer.h>
#include <aerospike/as_string.h>
#include <aerospike/as_list.h>
#include <aerospike/as_arraylist.h>
#include <aerospike/as_map.h>
#include <aerospike/as_hashmap.h>
#include <aerospike/as_stringmap.h>
#include <aerospike/as_val.h>

#include "../test.h"

/******************************************************************************
 * GLOBAL VARS
 *****************************************************************************/

extern aerospike * as;

/******************************************************************************
 * STATIC FUNCTIONS
 *****************************************************************************/


static bool key_basics_print_bins(const char * name, const as_val * value, void * udata)
{
	char * sval = as_val_tostring(value);
	info("bin: name=%s, value=%s", name, sval);
	free(sval);
	return true;
}

/******************************************************************************
 * TEST CASES
 *****************************************************************************/

TEST( key_basics_put , "put: (test,test,foo) = {a: 123, b: 'abc', c: 456, d: 'def', e: [1,2,3], f: {x: 7, y: 8, z: 9}}" ) {

	as_error err;
	as_error_reset(&err);
	
	as_arraylist list;
	as_arraylist_init(&list, 3, 0);
	as_arraylist_append_int64(&list, 1);
	as_arraylist_append_int64(&list, 2);
	as_arraylist_append_int64(&list, 3);
	
	as_hashmap map;
	as_hashmap_init(&map, 32);
	as_stringmap_set_int64((as_map *) &map, "x", 7);
	as_stringmap_set_int64((as_map *) &map, "y", 8);
	as_stringmap_set_int64((as_map *) &map, "z", 9);

	as_record r, * rec = &r;
	as_record_init(rec, 6);
	as_record_set_int64(rec, "a", 123);
	as_record_set_str(rec, "b", "abc");
	as_record_set_integer(rec, "c", as_integer_new(456));
	as_record_set_string(rec, "d", as_string_new("def",false));
	as_record_set_list(rec, "e", (as_list *) &list);
	as_record_set_map(rec, "f", (as_map *) &map);

	as_key key;
	as_key_init(&key, "test", "test", "foo");

	as_status rc = aerospike_key_put(as, &err, NULL, &key, rec);

	as_key_destroy(&key);

    info("bins: ");
    as_record_foreach(rec, key_basics_print_bins, NULL);

	as_record_destroy(rec);

	assert_int_eq( rc, AEROSPIKE_OK );
}

TEST( key_basics_put_key , "put_with_key: (test,test,foo_key) = <bytes>" ) {

	as_error err;
	as_error_reset(&err);

	int count = 20000;
	uint8_t *mybytes = alloca (count);
	memset(mybytes, count, count);

	as_record r, * rec = &r;
	as_record_init(rec, 1);
	as_record_set_rawp(rec, "a", mybytes, count, false);

	// Set up a as_policy_write object with SEND_KEY
	as_policy_write wpol;
	as_policy_write_init(&wpol);
	wpol.key = AS_POLICY_KEY_SEND;

	as_key key;
	as_key_init(&key, "test", "test", "foo_key");

	as_status rc = aerospike_key_put(as, &err, &wpol, &key, rec);
	assert_int_eq( rc, AEROSPIKE_OK );
	as_record_destroy(rec);

	as_error_reset(&err);
	as_record * rrec=NULL;
	rc = aerospike_key_get(as, &err, NULL, &key, &rrec);
	assert_int_eq( rc, AEROSPIKE_OK );

	as_record_foreach(rrec, key_basics_print_bins, NULL);

	as_key_destroy(&key);
	as_record_destroy(rrec);
}

TEST( key_basics_get , "get: (test,test,foo) = {a: 123, b: 'abc', c: 456, d: 'def', e: [1,2,3], f: {x: 7, y: 8, z: 9}}" ) {

	as_error err;
	as_error_reset(&err);

	as_record r, *rec = &r;
	as_record_init(&r, 0);

	as_key key;
	as_key_init(&key, "test", "test", "foo");

	as_status rc = aerospike_key_get(as, &err, NULL, &key, &rec);

	as_key_destroy(&key);

    info("bins: ");
    as_record_foreach(rec, key_basics_print_bins, NULL);
	
    assert_int_eq( rc, AEROSPIKE_OK );
    assert_int_eq( as_record_numbins(rec), 6 );
    
    assert_int_eq( as_record_get_int64(rec, "a", 0), 123 );
    assert_not_null( as_record_get_integer(rec, "a") );
	assert_int_eq( as_integer_toint(as_record_get_integer(rec, "a")), 123 );

    assert_string_eq( as_record_get_str(rec, "b"), "abc" );
	assert_not_null( as_record_get_string(rec, "b") );
	assert_string_eq( as_string_tostring(as_record_get_string(rec, "b")), "abc" );
    
    assert_int_eq( as_record_get_int64(rec, "c", 0), 456 );
    assert_not_null( as_record_get_integer(rec, "c") );
	assert_int_eq( as_integer_toint(as_record_get_integer(rec, "c")), 456 );

	assert_string_eq( as_record_get_str(rec, "d"), "def" );
	assert_not_null( as_record_get_string(rec, "d") );
	assert_string_eq( as_string_tostring(as_record_get_string(rec, "d")), "def" );

    as_list * list = as_record_get_list(rec, "e");
    assert_not_null( list );
    assert_int_eq( as_list_size(list), 3 );

    as_map * map = as_record_get_map(rec, "f");
    assert_not_null( map );
    assert_int_eq( as_map_size(map), 3 );

    as_record_destroy(rec);
}

TEST( key_basics_select , "select: (test,test,foo) = {a: 123, b: 'abc'}" ) {

	as_error err;
	as_error_reset(&err);

	as_record r, *rec = &r;
	as_record_init(&r, 0);

	as_key key;
	as_key_init(&key, "test", "test", "foo");

	const char * bins[3] = { "a", "b", NULL };

	as_status rc = aerospike_key_select(as, &err, NULL, &key, bins, &rec);

	as_key_destroy(&key);
    
    assert_int_eq( rc, AEROSPIKE_OK );
    assert_int_eq( as_record_numbins(rec), 2 );
    
    assert_int_eq( as_record_get_int64(rec, "a", 0), 123 );
    assert_not_null( as_record_get_integer(rec, "a") );
	assert_int_eq( as_integer_toint(as_record_get_integer(rec, "a")), 123 );

    assert_string_eq( as_record_get_str(rec, "b"), "abc" );
	assert_not_null( as_record_get_string(rec, "b") );
	assert_string_eq( as_string_tostring(as_record_get_string(rec, "b")), "abc" );
    
    assert_int_eq( as_record_get_int64(rec, "c", 0), 0 );
    assert_null( as_record_get_integer(rec, "c") );
	assert_null( as_record_get_str(rec, "d") );
	assert_null( as_record_get_string(rec, "d") );
    assert_null( as_record_get_list(rec, "e") );
    assert_null( as_record_get_map(rec, "f") );


    info("bins: ");
    as_record_foreach(rec, key_basics_print_bins, NULL);

    as_record_destroy(rec);
}

TEST( key_basics_exists , "exists: (test,test,foo)" ) {

	as_error err;
	as_error_reset(&err);

	as_key key;
	as_key_init(&key, "test", "test", "foo");

	as_record * rec = NULL;
	as_status rc = aerospike_key_exists(as, &err, NULL, &key, &rec);

	as_key_destroy(&key);

    assert_int_eq( rc, AEROSPIKE_OK );
	assert_not_null( rec );
	assert_true ( rec->gen != 0 );
	assert_true ( rec->ttl != 0 );
	
	as_record_destroy(rec);
}

TEST( key_basics_notexists , "not exists: (test,test,foozoo)" ) {

	as_error err;
	as_error_reset(&err);

	as_key key;
	as_key_init(&key, "test", "test", "foozoo");

	
	as_record * rec = NULL;
	as_status rc = aerospike_key_exists(as, &err, NULL, &key, &rec);

	as_key_destroy(&key);

    assert_int_eq( rc, AEROSPIKE_ERR_RECORD_NOT_FOUND );
	assert_null( rec );
	
	as_record_destroy(rec);
}

TEST( key_basics_remove , "remove: (test,test,foo)" ) {

	as_error err;
	as_error_reset(&err);

	as_key key;
	as_key_init(&key, "test", "test", "foo");

	as_status rc = aerospike_key_remove(as, &err, NULL, &key);

	as_key_destroy(&key);

    assert_true( rc == AEROSPIKE_OK || rc == AEROSPIKE_ERR_RECORD_NOT_FOUND );
}

TEST( key_basics_remove_generation , "remove generation: (test,test,foo)" ) {

	as_error err;
	as_error_reset(&err);

	as_policy_remove dpol;
	as_policy_remove_init(&dpol);
	dpol.gen = AS_POLICY_GEN_EQ;
	dpol.generation = 2;

	as_key key;
	as_key_init(&key, "test", "test", "foo");

	as_status rc = aerospike_key_remove(as, &err, &dpol, &key);

	assert_true( rc == AEROSPIKE_ERR_RECORD_GENERATION );

	dpol.generation = 1;
	rc = aerospike_key_remove(as, &err, &dpol, &key);

    assert_true( rc == AEROSPIKE_OK );

    as_key_destroy(&key);
}

TEST( key_basics_put_generation , "put generation: (test,test,foo)" ) {

	as_error err;
	as_error_reset(&err);

	as_key key;
	as_key_init(&key, "test", "test", "foo");

	as_record r, * rec = &r;
	as_record_init(rec, 1);
	as_record_set_int64(rec, "a", 123);

	as_status rc = aerospike_key_put(as, &err, NULL, &key, rec);

	assert_true( rc == AEROSPIKE_OK );

	as_record_destroy(rec);
	as_record_init(rec, 1);
	as_record_set_int64(rec, "a", 456);

	as_policy_write wpol;
	as_policy_write_init(&wpol);
	wpol.gen = AS_POLICY_GEN_EQ; // perform generation equality check on writes

	rec->gen = 2; // generation in database should be 1, so this should fail

	rc = aerospike_key_put(as, &err, &wpol, &key, rec);

	assert_true( rc == AEROSPIKE_ERR_RECORD_GENERATION );

	rec->gen = 1; // ... but this should succeed

	rc = aerospike_key_put(as, &err, &wpol, &key, rec);

	assert_true( rc == AEROSPIKE_OK );

	as_record_destroy(rec);
	as_record_init(rec, 1);
	as_record_set_nil(rec, "a"); // remove bin 'a' - causes record deletion

	rec->gen = 2; // generation in database should be 2, so this should succeed

	rc = aerospike_key_put(as, &err, &wpol, &key, rec);

	assert_true( rc == AEROSPIKE_OK );

	as_record_destroy(rec);

	// Verify the record is gone.
	rec = NULL;
	rc = aerospike_key_exists(as, &err, NULL, &key, &rec);

	assert_int_eq( rc, AEROSPIKE_ERR_RECORD_NOT_FOUND );
	assert_null( rec );

	as_key_destroy(&key);
}

TEST( key_basics_remove_notexists , "remove not exists: (test,test,foozoo)" ) {
	
	as_error err;
	as_error_reset(&err);
	
	as_key key;
	as_key_init(&key, "test", "test", "foozoo");
	
	as_status rc = aerospike_key_remove(as, &err, NULL, &key);
	
	as_key_destroy(&key);
	
    assert_true( rc == AEROSPIKE_ERR_RECORD_NOT_FOUND );
	assert_true( err.message[0] != 0 );
}

TEST( key_basics_operate , "operate: (test,test,foo) => {a: incr(321), b: append('def'), d: prepend('abc')}" ) {

	as_error err;
	as_error_reset(&err);

	as_operations ops;
	as_operations_inita(&ops, 3);
	as_operations_add_incr(&ops, "a", 321);
	as_operations_add_append_str(&ops, "b", "def");
	as_operations_add_prepend_str(&ops, "d", "abc");

	as_key key;
	as_key_init(&key, "test", "test", "foo");

	as_status rc = aerospike_key_operate(as, &err, NULL, &key, &ops, NULL);

	as_key_destroy(&key);

    assert_int_eq( rc, AEROSPIKE_OK );
}

TEST( key_basics_get2 , "get: (test,test,foo) = {a: 444, b: 'abcdef', d: 'abcdef'}" ) {

	as_error err;
	as_error_reset(&err);

	as_key key;
	as_key_init(&key, "test", "test", "foo");

	as_record r, *rec = &r;
	as_record_init(&r, 0);

	as_status rc = aerospike_key_get(as, &err, NULL, &key, &rec);

	as_key_destroy(&key);
    
    info("bins: ");
    as_record_foreach(rec, key_basics_print_bins, NULL);
	
    assert_int_eq( rc, AEROSPIKE_OK );
    assert_int_eq( as_record_numbins(rec), 6 );
    
    assert_int_eq( as_record_get_int64(rec, "a", 0), 444 );
    assert_not_null( as_record_get_integer(rec, "a") );
	assert_int_eq( as_integer_toint(as_record_get_integer(rec, "a")), 444 );

    assert_string_eq( as_record_get_str(rec, "b"), "abcdef" );
	assert_not_null( as_record_get_string(rec, "b") );
	assert_string_eq( as_string_tostring(as_record_get_string(rec, "b")), "abcdef" );
    
    assert_int_eq( as_record_get_int64(rec, "c", 0), 456 );
    assert_not_null( as_record_get_integer(rec, "c") );
	assert_int_eq( as_integer_toint(as_record_get_integer(rec, "c")), 456 );

	assert_string_eq( as_record_get_str(rec, "d"), "abcdef" );
	assert_not_null( as_record_get_string(rec, "d") );
	assert_string_eq( as_string_tostring(as_record_get_string(rec, "d")), "abcdef" );

    as_list * list = as_record_get_list(rec, "e");
    assert_not_null( list );
    assert_int_eq( as_list_size(list), 3 );

    as_map * map = as_record_get_map(rec, "f");
    assert_not_null( map );
    assert_int_eq( as_map_size(map), 3 );

    as_record_destroy(rec);
}
/******************************************************************************
 * TEST SUITE
 *****************************************************************************/

SUITE( key_basics, "aerospike_key basic tests" ) {
	// Remove at beginning to clear out record.
    suite_add( key_basics_put_key );
    suite_add( key_basics_remove );
	suite_add( key_basics_put );
	suite_add( key_basics_exists );
	suite_add( key_basics_notexists );
	// TODO - when we're sure we won't be running against older servers, let these in:
//	suite_add( key_basics_remove_generation );
//	suite_add( key_basics_put_generation );
	suite_add( key_basics_put );
	suite_add( key_basics_get );
	suite_add( key_basics_select );
	suite_add( key_basics_operate );
	suite_add( key_basics_get2 );
	suite_add( key_basics_remove );
	suite_add( key_basics_remove_notexists );
	suite_add( key_basics_notexists );
}
