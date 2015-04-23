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
	${PEP8} gstswitch
	@echo "gstswitch is pep8 clean!!"
	${PEP8} tests/unittests
	@echo "unittests are pep8 clean!!"
	${PEP8} tests/integrationtests
	@echo "integration tests are pep8 clean!!"

style: pep8 lint

imgurbash.sh:
	-wget http://imgur.com/tools/imgurbash.sh -O imgurbash.sh
	-chmod +x imgurbash.sh

unittests:
	@rm -f ../tools/*.c.gcov
	@rm -f ../tools/*.gcda
	@rm -rf reports
	@mkdir -p reports/coverage/unittests
	${PYTEST} --cov gstswitch --pep8 -v -s tests/unittests/
	#-@mv htmlcov/*.* reports/coverage/unittests

integration: imgurbash.sh
	@rm -f ../tools/*.c.gcov
	@rm -f ../tools/*.gcda
	@rm -rf reports
	@mkdir -p reports/coverage/integration
	${PYTEST} --cov gstswitch --pep8 -v -s tests/integrationtests/
	#-@mv htmlcov/*.* reports/coverage/integration

performance:
	${PYTEST} tests/performancetests/*.py -v -s
	make clean

test: unittests integration

clean:
	rm -f *.avi*
	rm -f *.data*
	rm -f *.html
	rm -f *.log
	rm -f *.sh*
	rm -f .coverage
	rm -f tests/integrationtests/*.data
	rm -rf .cache
	rm -rf reports
