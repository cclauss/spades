#include "contig_processor.hpp"
#include "config_struct.hpp"
// WTF: Include only what you're using
// Re: include.hpp IS used, utils under discussion
#include "include.hpp"
#include "utils.hpp"

#include "io/ireader.hpp"
#include "io/osequencestream.hpp"
#include "io/file_reader.hpp"
#include "io/single_read.hpp"
#include "path_helper.hpp"

namespace corrector {


void ContigProcessor::ReadContig() {
    io::FileReadStream frs(contig_file);
    io::SingleRead cur_read;
    frs >> cur_read;
    if (!frs.eof()) {
        WARN ("Non unique sequnce in one contig fasta!");
    }
    contig_name = cur_read.name();
    contig = cur_read.GetSequenceString();

    output_contig_file = path::append_path(path::parent_path(contig_file), path::basename(contig_file) +  ".ref.fasta");
    charts.resize(contig.length());
}

void ContigProcessor::UpdateOneRead(const SingleSamRead &tmp, MappedSamStream &sm) {
    unordered_map<size_t, position_description> all_positions;
    if (tmp.get_contig_id() < 0) {
        return;
    }
    string cur_s = sm.get_contig_name(tmp.get_contig_id());
    if (cur_s != contig_name) {
        return;
    }
    tmp.CountPositions(all_positions, contig.length());
    size_t error_num = 0;

    for (auto &pos : all_positions) {
        if ((int) pos.first >= 0 && pos.first < contig.length()) {
            charts[pos.first].update(pos.second);
            if (pos.second.FoundOptimal(contig[pos.first]) != var_to_pos[(int)contig[pos.first]]) {
                error_num ++;
            }
        }
    }

    if (error_num >= error_counts.size())
        error_counts[error_counts.size() - 1]++;
    else
        error_counts[error_num]++;
}

//returns: number of changed nucleotides;
size_t ContigProcessor::UpdateOneBase(size_t i, stringstream &ss, const unordered_map<size_t, position_description> &interesting_positions) {
    char old = (char) toupper(contig[i]);
    size_t maxi = charts[i].FoundOptimal(contig[i]);
    auto i_position = interesting_positions.find(i);
    if ( i_position != interesting_positions.end()) {
        size_t maxj = i_position->second.FoundOptimal(contig[i]);
        if (maxj != maxi) {
            DEBUG("Interesting positions differ with majority!");
            DEBUG("On position " << i << "  old: " << old << " majority: " << pos_to_var[maxi] << "interesting: " << pos_to_var[maxj]);
            if (corr_cfg::get().strat != strategy::majority_only)
                maxi = maxj;
        }
    }
    if (old != pos_to_var[maxi]) {
        DEBUG("On position " << i << " changing " << old << " to " << pos_to_var[maxi]);
        DEBUG(charts[i].str());
        if (maxi < Variants::Deletion) {
            ss << pos_to_var[maxi];
            return 1;
        } else if (maxi == Variants::Deletion) {
            return 1;
        } else if (maxi == Variants::Insertion) {
            string maxj = "";
            //first base before insertion;
            size_t new_maxi = var_to_pos[(int) contig[i]];
            int new_maxx = charts[i].votes[new_maxi];
            for (size_t k = 0; k < MAX_VARIANTS; k++) {
                if (new_maxx < charts[i].votes[k] && (k != Variants::Insertion) && (k != Variants::Deletion)) {
                    new_maxx = charts[i].votes[k];
                    new_maxi = k;
                }
            }
            ss << pos_to_var[new_maxi];
            int max_ins = 0;
            for (const auto &ic : charts[i].insertions) {
                if (ic.second > max_ins) {
                    max_ins = ic.second;
                    maxj = ic.first;
                }
            }
            DEBUG("most popular insertion: " << maxj);
            ss << maxj;
            if (old == maxj[0]) {
                return (int) maxj.length() - 1;
            } else {
                return (int) maxj.length();
            }
        } else {
            //something starnge happened
            WARN("While processing base " << i << " unknown decision was made");
            return 0;
        }
    } else {
        ss << old;
        return 0;
    }
}

void ContigProcessor::ProcessMultipleSamFiles() {
    error_counts.resize(max_error_num);
    DEBUG("working with " << sam_files.size() << " sublibs");
    for (const auto &sf : sam_files) {
        MappedSamStream sm(sf.first);
        while (!sm.eof()) {
            SingleSamRead tmp;
            sm >> tmp;

            UpdateOneRead(tmp, sm);
        }
        sm.close();
    }

    stringstream err_str;
    // WTF: err_str is only used for debug!
     // Re: yes. Why not?
    for (size_t i = 0; i < error_counts.size(); i++)
        err_str << error_counts[i] << " ";
    size_t interesting = ipp.FillInterestingPositions(charts);
    if (debug_info) {
        INFO("Error counts for contig " << contig_name << " in thread " << omp_get_thread_num() << " : " << err_str.str());
        INFO(interesting << " positions" <<  "are in consideration "  );
    } else {
        DEBUG("Error counts for contig " << contig_name << " : " << err_str.str());
    }
    for (const auto &sf : sam_files) {
        MappedSamStream sm(sf.first);
        while (!sm.eof()) {
            unordered_map<size_t, position_description> ps;
            if (sf.second == io::LibraryType::PairedEnd || sf.second == io::LibraryType::HQMatePairs) {
                PairedSamRead tmp;
                sm >> tmp;
                tmp.CountPositions(ps, contig.length());
            } else {
                SingleSamRead tmp;
                sm >> tmp;
                tmp.CountPositions(ps, contig.length());
            }
            TRACE("updating interesting read..");
            ipp.UpdateInterestingRead(ps);
        }
        sm.close();
    }
    ipp.UpdateInterestingPositions();
    unordered_map<size_t, position_description> interesting_positions = ipp.get_weights();
    stringstream s_new_contig;
    size_t total_changes = 0;
    for (size_t i = 0; i < contig.length(); i++) {
        DEBUG(charts[i].str());
        total_changes += UpdateOneBase(i, s_new_contig, interesting_positions);
    }
    if (debug_info || total_changes * 5 >  contig.length()) {
        INFO("Contig " << contig_name << " processed with " << total_changes << " changes");
    } else {
        DEBUG("Contig " << contig_name << " processed with " << total_changes << " changes");
    }
    contig_name = ContigRenameWithLength(contig_name, s_new_contig.str().length());
    io::osequencestream oss(output_contig_file);
    oss << io::SingleRead(contig_name, s_new_contig.str());

}

}
;
