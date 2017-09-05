// KaldiPyNNET2/nnet2-decoder.cc

// Copyright 2017  Sayint Product Group@ZEN3 (author: Anand Joseph)
// Derived from online2bin/online2-wav-nnet3-latgen-faster.cc, Copyright 2014  Johns Hopkins University (author: Daniel Povey)

// See ../../COPYING for clarification regarding multiple authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
// WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache 2 License for the specific language governing permissions and
// limitations under the License.


#include "nnet2-decoder.h"

namespace kaldi {


    using namespace fst;

    NNET2Decoder::NNET2Decoder(std::string config_file, std::string nnet2_rxfilename, std::string fst_rxfilename,
                               std::string word_boundary_file) {


        po.Register("chunk-length", &chunk_length_secs,
                    "Length of chunk size in seconds, that we process.  Set to <= 0 "
                            "to use all input in one chunk.");
        po.Register("word-symbol-table", &word_syms_rxfilename,
                    "Symbol table for words [for debug output]");
        po.Register("do-endpointing", &do_endpointing,
                    "If true, apply endpoint detection");
        po.Register("online", &online,
                    "You can set this to false to disable online iVector estimation "
                            "and have all the data for each utterance used, even at "
                            "utterance start.  This is useful where you just want the best "
                            "results and don't care about online operation.  Setting this to "
                            "false has the same effect as setting "
                            "--use-most-recent-ivector=true and --greedy-ivector-extractor=true "
                            "in the file given to --ivector-extraction-config, and "
                            "--chunk-length=-1.");

        feature_config.Register(&po);
        nnet2_decoding_config.Register(&po);
        endpoint_config.Register(&po);

        po.ReadConfigFile(config_file);

        feature_config.Register(&po);
        nnet2_decoding_config.Register(&po);
        endpoint_config.Register(&po);

        feature_info = new OnlineNnet2FeaturePipelineInfo(feature_config);


        if (!online) {
            feature_info->ivector_extractor_info.use_most_recent_ivector = true;
            feature_info->ivector_extractor_info.greedy_ivector_extractor = true;
            chunk_length_secs = -1.0;
        }

        trans_model = new TransitionModel;
        nnet = new nnet2::AmNnet;
        {
            bool binary;
            Input ki(nnet2_rxfilename, &binary);
            trans_model->Read(ki.Stream(), binary);
            nnet->Read(ki.Stream(), binary);
        }

        decode_fst = ReadFstKaldiGeneric(fst_rxfilename);

        WordBoundaryInfoNewOpts opts;
        opts.Register(&po);
        word_boundary_info = new WordBoundaryInfo(opts, word_boundary_file);

        if (word_syms_rxfilename != "")
            if (!(word_syms = fst::SymbolTable::ReadText(word_syms_rxfilename)))
                KALDI_ERR << "Could not read symbol table from file "
                          << word_syms_rxfilename;


    }

    std::string NNET2Decoder::decode(std::string filename) {
        std::ifstream audio_stream(filename, std::ios_base::binary);


        WaveData wave_data = WaveData();
        wave_data.Read(audio_stream);
        SubVector<BaseFloat> data(wave_data.Data(), 0);

        float samp_freq = wave_data.SampFreq();

        return this->decode_seg(data, samp_freq, 0.0);


    }

    std::string NNET2Decoder::decode(BaseFloat *audio, int samp_freq, int length, float samp_offset) {

        SubVector<BaseFloat> data(audio,length);

        return this->decode_seg(data, samp_freq, samp_offset);

    }

    std::string NNET2Decoder::decode_seg(SubVector<BaseFloat> data, float samp_freq, float offset = 0.0) {


        int32 num_done = 0, num_err = 0;
        double tot_like = 0.0;
        int64 num_frames = 0;
        double likelihood;
        std::string decoded_string = "";


        std::vector<std::pair<int32, BaseFloat> > delta_weights;


        int32 chunk_length;
        if (chunk_length_secs > 0) {
            chunk_length = int32(samp_freq * chunk_length_secs);
            if (chunk_length == 0) chunk_length = 1;
        } else {
            chunk_length = std::numeric_limits<int32>::max();
        }


        std::cout << "Sampling Freq: " << samp_freq << std::endl;


        OnlineIvectorExtractorAdaptationState adaptation_state(
                feature_info->ivector_extractor_info);
        OnlineNnet2FeaturePipeline *feature_pipeline = new OnlineNnet2FeaturePipeline(*feature_info);
        feature_pipeline->SetAdaptationState(adaptation_state);


        OnlineSilenceWeighting silence_weighting(
                *trans_model,
                feature_info->silence_weighting_config);

        std::cout << "Initializing Decoder" << std::endl;
        SingleUtteranceNnet2Decoder decoder(nnet2_decoding_config,
                                            *trans_model,
                                            *nnet,
                                            *decode_fst,
                                            feature_pipeline);


        feature_pipeline->AcceptWaveform(samp_freq, data);
        feature_pipeline->InputFinished();
        if (silence_weighting.Active() &&
            feature_pipeline->IvectorFeature() != NULL) {
            silence_weighting.ComputeCurrentTraceback(decoder.Decoder());
            silence_weighting.GetDeltaWeights(
                    feature_pipeline->IvectorFeature()->NumFramesReady(),
                    &delta_weights);
            feature_pipeline->IvectorFeature()->UpdateFrameWeights(
                    delta_weights);
        }
        std::cout << "Decoding Audio" << std::endl;
        decoder.AdvanceDecoding();


        decoder.FinalizeDecoding();

        CompactLattice clat;
        bool end_of_utterance = true;
        std::cout << "Obtaining Lattice" << std::endl;
        decoder.GetLattice(end_of_utterance, &clat);


        if (clat.NumStates() == 0) {
            KALDI_WARN << "Empty lattice.";
            return false;
        }

        CompactLattice best_path_clat, aligned_clat;
        std::cout << "Computing Shortest Path" << std::endl;
        CompactLatticeShortestPath(clat, &best_path_clat);

        Lattice best_path_lat;
        ConvertLattice(best_path_clat, &best_path_lat);

        LatticeWeight weight;

        std::vector<int32> alignment;
        std::vector<int32> words;
        std::vector<int32> times, lengths;

        std::cout << "Computing Word Alignments" << std::endl;
        WordAlignLattice(best_path_clat, *trans_model, *word_boundary_info, 0, &aligned_clat);
        CompactLatticeToWordAlignment(aligned_clat, &words, &times, &lengths);


        BaseFloat frame_shift = 0.01;

        std::ostringstream stringStream;
        stringStream << "{\"words\": [";
        bool insert_commas = false;
        if (word_syms != NULL) {
            for (size_t i = 0; i < words.size(); i++) {
                std::string s = word_syms->Find(words[i]);
                if (s == "")
                    KALDI_ERR << "Word-id " << words[i] << " not in symbol table.";
                if (words[i] != 0) {
                    decoded_string += s + ' ';
                    std::cout << (frame_shift * times[i]) << ' '
                              << (frame_shift * (times[i] + lengths[i])) << ' ' << s << std::endl;
                    if (insert_commas) {
                        stringStream << ", ";
                    }
                    stringStream << "{\"start\":" << (frame_shift * times[i] + offset) << ", "
                                 << "\"end\":" << (frame_shift * (times[i] + lengths[i]) + offset) << ", "
                                 << "\"word\":\"" << s << "\"}";
                    insert_commas = true;
                }

            }
            std::cerr << std::endl;
        }
        stringStream << "], \"text\":\"" << decoded_string << "\"}";

        delete feature_pipeline;
        return stringStream.str();

    }


}
