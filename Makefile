include $(KALDI_ROOT)/src/kaldi.mk
LDFLAGS += -Llib -lkaldi-decoder \
		  -lkaldi-lat   -lkaldi-fstext   -lkaldi-hmm     -lkaldi-feat    -lkaldi-transform \
		  -lkaldi-gmm   -lkaldi-tree     -lkaldi-util     -lkaldi-matrix \
		  -lkaldi-base  -lkaldi-nnet3    -lkaldi-online2

all: decode/nnetdecode.so

decode/nnetdecode.so:	decode/nnetdecode.pyx decode/nnet2-decoder.cpp decode/nnet2-decoder.h
	CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" python3 setup.py build_ext --inplace

clean:
	rm -f decode/nnetdecode.cpp decode/nnetdecode.so decode/*.pyc
	rm -rf build

