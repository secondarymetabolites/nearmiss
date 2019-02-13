all: build

build: source/tree.c source/sais.h source/sais.c
	python setup.py develop 

coverage: build
	rm -rf cover .coverage
	coverage run --source nearmiss -m pytest
	coverage html -d cover
	coverage report


clean:
	rm -rf *.egg-info build dist .pytest_cache .cache cover
	find . -name '__pycache__' | xargs rm -rf
	find . -name '*.pyc' | xargs rm -rf
	find . -name '*.so' | xargs rm -rf
