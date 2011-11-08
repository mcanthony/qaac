#include <ALACDecoder.h>
#include "shared_ptr.h"
#include "iointer.h"
#include "mp4v2wrapper.h"

class ALACSource:
    public ISource, public ITagParser, public PartialSource<ALACSource>
{
    struct DecodeBuffer {
	uint32_t nsamples;
	uint32_t done;
	std::vector<uint8_t> v;

	DecodeBuffer(): nsamples(0), done(0) {}
	void reset() { nsamples = done = 0; }
	uint32_t rest() { return nsamples - done; }
	void advance(uint32_t n) {
	    done += n;
	    if (done >= nsamples) done = nsamples = 0;
	}
    };
    uint32_t m_track_id;
    uint64_t m_position;
    x::shared_ptr<ALACDecoder> m_decoder;
    std::map<uint32_t, std::wstring> m_tags;
    SampleFormat m_format;
    MP4FileX m_file;
    DecodeBuffer m_buffer;
public:
    ALACSource(const std::wstring &path);
    uint64_t length() const { return getDuration(); }
    const SampleFormat &getSampleFormat() const { return m_format; }
    const std::vector<uint32_t> *getChannelMap() const { return 0; }
    size_t readSamples(void *buffer, size_t nsamples);
    void skipSamples(int64_t count) { m_position += count; }
    const std::map<uint32_t, std::wstring> &getTags() const { return m_tags; }
    const std::vector<std::pair<std::wstring, int64_t> >
	*getChapters() const { return 0; }
private:
    void fetchTags();
};