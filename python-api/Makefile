.PHONY: lint pep8 style-check unittests integration performance test clean

PYTHONVERSION := 3.4

PYTEST := py.test-${PYTHONVERSION}
PYLINT := python${PYTHONVERSION} $(shell which pylint)
# Keep the --ignore in sync with setup.cfg
PEP8 := python${PYTHONVERSION} $(shell which pep8) --ignore=E402

lint: export PYTHONWARNINGS := ignore
lint: export PYTHONPATH := ${PYTHONPATH}:${PWD}
lint:
	@rm -rf tests/unittests/__pycache__
	@rm -rf tests/integrationtests/__pycache__
	@rm -rf reports
	@echo PYTHONWARNINGS=$$PYTHONWARNINGS
	@echo PYTHONPATH=$$PYTHONPATH
	${PYLINT} --rcfile=.pylintrc_gstswitch gstswitch
	${PYLINT} --rcfile=.pylintrc_unittests tests/unittests/*
	${PYLINT} --rcfile=.pylintrc_integrationtests tests/integrationtests/*.py
	@echo "Code style is okay!!"

pep8:
	# E402 module level import not at top of file
	#  triggered by sys.path.insert(...) before gstswitch-imports
	${PEP8} --ignore=E402 gstswitch
	@echo "gstswitch is pep8 clean!!"
	${PEP8} --ignore=E402 tests/unittests
	@echo "unittests are pep8 clean!!"
	${PEP8} --ignore=E402 tests/integrationtests
	@echo "integration tests are pep8 clean!!"

style: pep8 lint

imgurbash.sh:
	-wget http://imgur.com/tools/imgurbash.sh -O imgurbash.sh
	-chmod +x imgurbash.sh

unittests:
	@rm -f ../tools/*.c.gcov
	@rm -f ../tools/*.gcda
	${PYTEST} --cov gstswitch --cov-report html -v tests/unittests/ -s
	mkdir -p reports/coverage/
	rm -rf reports/coverage/unittests-${PYTHONVERSION}
	mv htmlcov reports/coverage/unittests-${PYTHONVERSION}

integration: imgurbash.sh
	@rm -f ../tools/*.c.gcov
	@rm -f ../tools/*.gcda
	${PYTEST} --cov gstswitch --cov-report html -v tests/integrationtests/
	mkdir -p reports/coverage/
	rm -rf reports/coverage/integration-${PYTHONVERSION}
	mv htmlcov reports/coverage/integration-${PYTHONVERSION}

performance:
	${PYTEST} tests/performancetests/*.py -v -s
	make clean

test: unittests integration

clean:
	rm -f gstswitch/*.pyc
	rm -rf gstswitch/__pycache__
	rm -f tests/unittests/*.pyc
	rm -rf tests/unittests/__pycache__
	rm -f tests/integrationtests/*.pyc
	rm -rf tests/integrationtests/__pycache__
	rm -f *.avi*
	rm -f *.data*
	rm -f *.html
	rm -f *.log
	rm -f *.sh*
	rm -f .coverage
	rm -f tests/integrationtests/*.data
	rm -rf .cache
	rm -rf reports
