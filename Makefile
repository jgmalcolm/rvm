LDFLAGS +=
CFLAGS += -g -ansi -Wall -I$(HOME)/src/c/include -I. -DDEBUG

TESTS = abort basic modify_overlap multi-abort multi multi_trans \
        remap truncate destroy_nonexistant basic8 basic9 basic10 basic11 \
        bad_rvm_unmap_01 bad_rvm_unmap_02 bad_rvm_unmap_03 \
        semantics_01 semantics_02 rvm_map_01 \
		test_about_to_modify1 test_about_to_modify2 \
		test_about_to_modify3 test_about_to_modify4 \
		test_about_to_modify5 test_about_to_modify6 \
		test_about_to_modify7 test_about_to_modify8 \
		test_truncate1 test_truncate2 \
		test_multi1 test_multi2 test_multi4

TESTS_BIN = $(patsubst %, tests/%, $(TESTS))


all: run $(TESTS_BIN)

run: tests/abort
	./$^

tests/abort: rvm.o ll.o
tests/basic: rvm.o ll.o
tests/modify_overlap: rvm.o ll.o
tests/multi-abort: rvm.o ll.o
tests/multi: rvm.o ll.o
tests/multi_trans: rvm.o ll.o
tests/remap: rvm.o ll.o
tests/truncate: rvm.o ll.o
tests/destroy_nonexistant: rvm.o ll.o
tests/basic8: rvm.o ll.o
tests/basic9: rvm.o ll.o
tests/basic10: rvm.o ll.o
tests/basic11: rvm.o ll.o
tests/bad_rvm_unmap_01: rvm.o ll.o
tests/bad_rvm_unmap_02: rvm.o ll.o
tests/bad_rvm_unmap_03: rvm.o ll.o
tests/semantics_01: rvm.o ll.o
tests/semantics_02: rvm.o ll.o
tests/test_about_to_modify1: rvm.o ll.o
tests/test_about_to_modify2: rvm.o ll.o
tests/test_about_to_modify3: rvm.o ll.o
tests/test_about_to_modify4: rvm.o ll.o
tests/test_about_to_modify5: rvm.o ll.o
tests/test_about_to_modify6: rvm.o ll.o
tests/test_about_to_modify7: rvm.o ll.o
tests/test_about_to_modify8: rvm.o ll.o
tests/test_truncate1: rvm.o ll.o
tests/test_truncate2: rvm.o ll.o
tests/test_multi1: rvm.o ll.o
tests/test_multi2: rvm.o ll.o
tests/test_multi4: rvm.o ll.o
tests/rvm_map_01: rvm.o ll.o

clean:
	rm -f *.o $(TESTS_BIN)
