#
from distutils.core import setup
from distutils.extension import Extension
import os,sys
import numpy

try:
    from Cython.Distutils import build_ext
except ImportError:
    use_cython = False
else:
    use_cython = True

cmdclass = { }
ext_modules = [ ]

if os.getenv("KALDI_ROOT") is not None:
    KALDI_ROOT=os.getenv("KALDI_ROOT")
    if not os.path.exists(KALDI_ROOT+"/src/lib/libkaldi-decoder.so"):
        print("ERROR: Could not find compiled kaldi libraries at {}/src/lib. Check if Kaldi is available on machine, KALDI_ROOT environment variable is set and Kaldi is configure with '--shared'".format(KALDI_ROOT),file=sys.stderr)
        exit(1)
else:
    print("ERROR: KALDI_ROOT variable is not set. Check if Kaldi is available on machine, KALDI_ROOT environment variable is set and Kaldi is configure with '--shared'",file=sys.stderr)
    exit(1)


if os.getenv("LD_LIBRARY_PATH") is not None:
    if (KALDI_ROOT+"/src/lib") not in os.getenv("LD_LIBRARY_PATH"):
        print("WARNING: LD_LIBRARY_PATH does not point to valid Kaldi Library",file=sys.stderr)
        print("WARNING: Do LD_LIBRARY_PATH={}/tools/openfst/lib:{}/src/lib".format(KALDI_ROOT,KALDI_ROOT),file=sys.stderr)





compile_args = ['-std=c++11', '-DHAVE_EXECINFO_H=1', '-DHAVE_CXXABI_H', '-DHAVE_ATLAS',
		'-pthread', '-DKALDI_DOUBLEPRECISION=0', '-Wno-sign-compare']
linker_args = ["-L{}/tools/openfst/lib".format(KALDI_ROOT),
				   "-L{}/src/lib".format(KALDI_ROOT),
				   "-lfst", "/usr/lib/atlas-base/libatlas.so", "-lm", 
				   "-lpthread", "-ldl", "-lkaldi-decoder", "-lkaldi-lat",
				   "-lkaldi-fstext", "-lkaldi-hmm", "-lkaldi-feat", 
				   "-lkaldi-transform", "-lkaldi-gmm", "-lkaldi-tree", 
				   "-lkaldi-util", "-lkaldi-matrix", "-lkaldi-base", "-lkaldi-nnet3",  
				   "-lkaldi-online2"]

if use_cython:
    ext_modules += [
        Extension("decode.nnetdecode", 
                  sources  = [ "decode/nnetdecode.pyx", "decode/nnet2-decoder.cpp" ],
		  extra_compile_args=compile_args,
		  extra_link_args= linker_args,
                  language = "c++")]
    cmdclass.update({ 'build_ext': build_ext })
else:
    ext_modules += [
        Extension("decode.nnetdecode", 
                  sources  = [ "decode/nnetdecode.cpp", "decode/nnet2-decoder.cpp" ],
		  extra_compile_args = compile_args,
		  extra_link_args = linker_args,
                  language = "c++")]

setup(
    name                 = 'KaldiPy',
    version              = '0.0.1',
    description          = 'A Python Wrapper for the NNET2 Decoder in Kaldi',
    long_description     = open('README.md').read(),
    author               = 'Anand Joseph',
    author_email         = 'anand@teletext-holidays.co.uk',
    maintainer           = 'Anand Joseph',
    maintainer_email         = 'anand@teletext-holidays.co.uk',
    packages             = ['KaldiPy'],
    cmdclass             = cmdclass,
    ext_modules          = ext_modules,
    include_dirs         = [numpy.get_include(), '{}/src'.format(KALDI_ROOT), '{}/tools/openfst/include'.format(KALDI_ROOT), '{}/tools/ATLAS/include'.format(KALDI_ROOT)  ],
    classifiers          = [
                               'Development Status :: 2 - Pre-Alpha',
                               'Operating System :: POSIX :: Linux',
                               'License :: OSI Approved :: Apache Software License',
                               'Programming Language :: Python :: 2',
                               'Programming Language :: Python :: 2.7',
                               'Programming Language :: Cython',
                               'Programming Language :: C++',
                               'Intended Audience :: Developers',
                               'Topic :: Software Development :: Libraries :: Python Modules',
                               'Topic :: Multimedia :: Sound/Audio :: Speech'
                           ],
    license              = 'Apache',
    keywords             = 'kaldi-asr',
    )


