from distutils.core import setup

setup(name='zyre',
	  description="""an open-source framework for proximity-based P2P apps""",
	  version='0.1',
	  packages=['zyre'],
	  package_dir={'': 'bindings/python'},
)
