import cython
from libcpp.string cimport string
from libcpp.vector cimport vector
import struct
import json
import numpy as np
cimport numpy as np


cdef extern from "nnet2-decoder.h" namespace "kaldi":
    cdef cppclass NNET2Decoder:
        NNET2Decoder(string, string, string, string) except+
        string decode(string) except+
        string decode(float *, float fs, float length, float offset) except+


cdef class PyNNET2Decoder:
    cdef NNET2Decoder*ks
    cdef string              model, graph, config, word_bound

    def __cinit__(self, string config, string model, string graph, string word_bound):
        self.config = config
        self.model = model
        self.graph = graph
        self.word_bound = word_bound

        self.ks = new NNET2Decoder(config, model, graph, word_bound)

    def __dealloc__(self):
        del self.ks

    def decode_file(self, string audio_file):
        return json.loads(self.ks.decode(audio_file).decode())


    def decode_array (self, np.ndarray[float, ndim=1, mode="c"] audio not None, float fs, float offset=0.0):
        return json.loads(self.ks.decode(&audio[0],fs, audio.size, 0.0).decode())


    def decode_array_seg (self, np.ndarray[float, ndim=1, mode="c"] audio not None, float fs, float seg_start, float seg_end):

        array_seg=audio[ int(np.floor(seg_start*fs)) : int(np.floor(seg_end*fs)) ].copy()
        return self.decode_array(array_seg,fs,seg_start)

