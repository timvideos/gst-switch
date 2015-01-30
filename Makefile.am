SUBDIRS = plugins tools tests/unit tests

EXTRA_DIST = autogen.sh

ACLOCAL_AMFLAGS = -I build

include tests/names.mk
test: update-test-suite clean-test-data $(TESTS)
demo: $(UI_TESTS)

show-tests:
	@printf "Available tests:\n\t$(TESTS)\n"
	@printf "And these tests demonstrate the UI:\n\t$(UI_TESTS)\n"
	@printf "You may want to do 'make test-*' for specific test.\n"

$(TESTS) $(UI_TESTS):
	@$(MAKE) -C tests $@

update-test-suite: tests/test-switch-server tests/test-fd-leaks
	$(MAKE)

tests/test-switch-server: tests/test_switch_server.c \
  tools/*.h tools/*.c
tests/test-fd-leaks: tests/test_fd_leaks.c \
  tools/*.h tools/*.c

clean-test-data:
	rm -f "tests/test-recording "*" "*.data
	rm -f "tests/test-server-"*.log

valgrind:
	./tests/run_server_in_valgrind.sh $(TESTS)

if GPROF_ENABLED
perf:
	echo "TODO: program profiling"
endif

if GCOV_ENABLED
coverage:
	$(MAKE) -C tools coverage
	$(MAKE) -C tests/unit coverage
endif

style:
	git status || true
	git diff || true
	git diff-files; echo $$?
	git diff-files --quiet || (echo "Can only run make style on clean repo"; git status; git diff; git diff-files; exit 1)
	@echo "--------------------------------------------------------"
	@echo "--------------------------------------------------------"
	for i in $$(find -name \*.c -o -name \*.h); do ./scripts/indent $$i; done
	git diff
	@echo "--------------------------------------------------------"
	@echo "--------------------------------------------------------"
	@git diff-files --quiet || (echo "Style errors found!"; exit 1)

integration:
	$(MAKE) -C python-api integration
	
unittests:
	$(MAKE) -C tests/unit check
