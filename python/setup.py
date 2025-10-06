import os.path

from setuptools import setup, find_packages

######################
# For extremely simple modules, you can just fill these top variables in
# for more complex items, you'll want to modify the setup directly
toolname = "fj_sync"
######################

version_file = os.path.join(
            os.path.dirname(__file__),
            "fj_sync",
            "_version.py")
encoding_kwargs = {'encoding': 'utf-8',}
kwargs = {}

exec(open(version_file, 'rt', **encoding_kwargs).read())
package_name = "fj_sync"

# build package_dir first
packages=find_packages(exclude=['tests*', 'docs*']),

setup(name=package_name,
      version=__version__.get_full_version(),
      description='file jump synchronizer',
      long_description='Python module to access FileJump cloud storage',
      author='Lev Zlotin',
      author_email='lev.zlotin@gmail.com',
      packages=packages,
      include_package_data=True,
      install_requires=[
          "requests",
          "requests_toolbelt",
      ],
    entry_points = {
    'console_scripts': [],
        },
      **kwargs,
)
