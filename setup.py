#!/usr/bin/env python

from distutils.core import setup, Extension
from os import environ as env

version = '4.0'

undef_macros=[
   'PYTHON_CA_USE_GIL_API',
   'PYTHON_CA_NODEBUG',
   'PYTHON_CA_HELPERS',
   'PYTHON_CA_DEVCORE'
   ]
   
define_macros=[
   ('PYTHON_CA_VERSION',          '\"%s\"' % version),
   ('PYTHON_CA_DEBUGLEVEL',       '0'               ), # ca.DEBUGLEVEL
   ('PYTHON_CA_SYNCT',            '5.0'             ), # ca.SYNCT
   ('PYTHON_CA_ENUM_AS_STRING',   '1'               ), # ca.ENUM_AS_STRING
   ('PYTHON_CA_ATEXIT',           None              ), # atexit->ca_context_destroy
   ]


try: epics=env['EPICS']
except: raise SystemExit('EPICS (base) is not defined')

try: host_arch=env['HOST_ARCH']
except: raise SystemExit('HOST_ARCH is not defined')

try: epics_host_arch=env['EPICS_HOST_ARCH']
except: raise SystemExit('EPICS_HOST_ARCH is not defined')

extra_compile_args=[]

setup(name='python3-ca',
      version=version,
      description='Channel Access for Python',
      long_description='EPICS Channel Access for Python.',
      author='Matthieu Bec',
      author_email='mdcb808@gmail.com',
      url='https://github.com/mdcb/python-ca',
      license='GPL',
      ext_modules=[
         Extension(
            name='ca',
            sources = [
               'src/ca_module_utils.c',
               'src/ca_module_admin.cpp',
               'src/pv_addCb.c',
               'src/pv_call.c',
               'src/pv_cond.c',
               'src/pv_delete.c',
               'src/pv_enum.c',
               'src/pv_get.c',
               'src/pv_gettersetter.c',
               'src/pv_init.c',
               'src/pv_monitor.c',
               'src/pv_put.c',
               'src/pv_repr.c',
               'src/pv_str.c',
               #XXX 'src/pv_buffer.c',
               'src/ca_module.c',
              ],
            include_dirs=[
               epics+'/include',
               epics +'/include/os/'+host_arch,
               epics +'/include/compiler/gcc',
               ],
            undef_macros=undef_macros,
            define_macros=define_macros,
            library_dirs=[epics+'/lib/'+epics_host_arch],
            libraries = ['m','ca','Com'],
            #runtime_library_dirs = [epics+'/lib/'+epics_host_arch],
            extra_compile_args = extra_compile_args,
            )
         ],
      #XXX py_modules=['ca_gtk'],
      #XXX data_files=[('share/python-ca', ['glade/ca_admin.glade']),
      #XXX ],
      )
     
