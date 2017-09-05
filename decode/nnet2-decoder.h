//
// Created by anand on 3/6/17.
//

#ifndef SRC_NNET2_DECODER_H
#define SRC_NNET2_DECODER_H

#include "feat/wave-reader.h"
#include "online2/online-nnet2-decoding.h"
#include "online2/online-nnet2-feature-pipeline.h"
#include "online2/onlinebin-util.h"
#include "online2/online-timing.h"
#include "online2/online-endpoint.h"
#include "fstext/fstext-lib.h"
#include "lat/lattice-functions.h"
#include "util/kaldi-thread.h"
#include "lat/word-align-lattice.h"
#include <utility>

namespace kaldi {


    //using namespace kaldi;
    using namespace fst;

    typedef kaldi::int32 int32;
    typedef kaldi::int64 int64;


    class NNET2Decoder {
    private:
        const char *usage = "Reads in wav file(s) and simulates online decoding with neural nets\n"
                "(nnet2 setup), with optional iVector-based speaker adaptation and\n"
                "optional endpointing.  Note: some configuration values and inputs are\n"
                "set via config files whose filenames are passed as options\n"
                "\n"
                "Usage: online2-wav-nnet2-latgen-faster [options] <nnet2-in> <fst-in> "
                "<spk2utt-rspecifier> <wav-rspecifier> <lattice-wspecifier>\n"
                "The spk2utt-rspecifier can just be <utterance-id> <utterance-id> if\n"
                "you want to decode utterance by utterance.\n"
                "See egs/rm/s5/local/run_online_decoding_nnet2.sh for example\n"
                "See also online2-wav-nnet2-latgen-threaded\n";

        ParseOptions po = ParseOptions(usage);

        OnlineNnet2FeaturePipelineInfo *feature_info = NULL;


        std::string word_syms_rxfilename;

        OnlineEndpointConfig endpoint_config;

        WordBoundaryInfo *word_boundary_info;

        // feature_config includes configuration for the iVector adaptation,
        // as well as the basic features.
        OnlineNnet2FeaturePipelineConfig feature_config;
        OnlineNnet2DecodingConfig nnet2_decoding_config;

        BaseFloat chunk_length_secs = 0.05;
        bool do_endpointing = false;
        bool online = true;


        fst::Fst<fst::StdArc> *decode_fst = NULL;
        fst::SymbolTable *word_syms = NULL;
        TransitionModel *trans_model = NULL;
        nnet2::AmNnet *nnet = NULL;


        void GetDiagnosticsAndPrintOutput(const std::string &utt,
                                          const fst::SymbolTable *word_syms,
                                          const CompactLattice &clat,
                                          int64 *tot_num_frames,
                                          double *tot_like);


        std::string decode_seg(SubVector<BaseFloat> data, float samp_freq, float samp_offset);


    public:
        NNET2Decoder(std::string config_file, std::string nnet2_rxfilename, std::string fst_rxfilename,
                     std::string word_boundary_file);

        std::string decode(std::string filename);

        std::string decode(BaseFloat *audio, int samp_freq, int length, float samp_offset);


    };
}
#endif //SRC_NNET2_DECODER_H
