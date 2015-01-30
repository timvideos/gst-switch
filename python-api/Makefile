.PHONY: lint unittests pytest integration test clean all

lint: export PYTHONWARNINGS := ignore
lint: export PYTHONPATH := ${PYTHONPATH}:${PWD}
lint:
	@rm -rf tests/unittests/__pycache__
	@rm -rf tests/integrationtests/__pycache__
	@rm -rf reports
	@echo PYTHONWARNINGS=$$PYTHONWARNINGS
	@echo PYTHONPATH=$$PYTHONPATH
	pylint --rcfile=.pylintrc_gstswitch gstswitch
	pylint --rcfile=.pylintrc_unittests tests/unittests/*
	pylint --rcfile=.pylintrc_integrationtests tests/integrationtests/*.py
	@echo "Code style is okay!!"

unittests:
	@rm -rf reports
	@mkdir -p reports/coverage/unittests
	py.test --cov gstswitch tests/unittests/ --pep8 -v -s

pep8:
	pep8 gstswitch
	@echo "gstswitch is pep8 clean!!"
	pep8 tests/unittests
	@echo "unittests are pep8 clean!!"
	pep8 tests/integrationtests
	@echo "integration tests are pep8 clean!!"

imgurbash.sh:
	-wget http://imgur.com/tools/imgurbash.sh -O imgurbash.sh
	-chmod +x imgurbash.sh

integration: imgurbash.sh
	@rm -f ../tools/*.c.gcov
	@rm -f ../tools/*.gcda
	@rm -rf reports
	@mkdir -p reports/coverage/integration
	py.test --cov gstswitch tests/integrationtests/ --pep8 -s -v 
	-@mv htmlcov/*.* reports/coverage/integration

performance:
	py.test tests/performancetests/*.py -v -s
	make clean

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

test: lint unittests integration clean
