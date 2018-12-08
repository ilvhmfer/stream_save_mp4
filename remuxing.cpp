
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

extern "C" {
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
}

#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"swresample.lib")
#pragma comment(lib,"swscale.lib")



static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt, const char *tag)
{
    /*AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    printf("%s: pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
           tag,
           av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
           av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
           av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
           pkt->stream_index);*/
}

static FILE *fp_open;  // only for test


int fill_iobuffer(void *opaque, uint8_t *buf, int buf_size)
{
	int ret = -1;

	if (!feof(fp_open))
	{
		int true_size = fread(buf, 1, buf_size, fp_open);  // only for test
		return true_size;
	}
	else
		return -1;

	/*if (g_stream_buf.size() == 0)
		return 0;

	STREAM_BUF strbuf = g_stream_buf.front();
	ret = strbuf.bufSize;
	if (strbuf.bufSize > 0) {
		memcpy(buf, strbuf.pAddr, strbuf.bufSize);
		delete [] strbuf.pAddr;
		strbuf.pAddr = nullptr;
	}

	g_stream_buf.pop();*/

	return ret;
}

#define IO_BUFFER_SIZE 200000

//int main(int argc, char **argv)
int save_mp4_new(const char* out_filename, int totalFrame)
{
	fp_open = fopen("test.264", "rb");   // only for test

	AVInputFormat *ifmt = NULL;
	AVOutputFormat *ofmt = NULL;
	
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
    AVPacket pkt;
    //const char *in_filename, *out_filename;
    int ret, i;
    int stream_index = 0;
    int *stream_mapping = NULL;
    int stream_mapping_size = 0;
	const char *in_filename = NULL;

	ifmt_ctx = avformat_alloc_context();

	unsigned char *iobuffer = (unsigned char *)av_malloc(IO_BUFFER_SIZE);
	AVIOContext *avio = avio_alloc_context(iobuffer, IO_BUFFER_SIZE, 0, NULL, fill_iobuffer, NULL, NULL);
	ifmt_ctx->pb = avio;

	ifmt = av_find_input_format("h264");
	
	do {
    if ((ret = avformat_open_input(&ifmt_ctx, NULL, ifmt, 0)) < 0) {
        fprintf(stderr, "Could not open input file '%s'", in_filename);
        goto end;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        fprintf(stderr, "Failed to retrieve input stream information");
        goto end;
    }

    av_dump_format(ifmt_ctx, 0, in_filename, 0);

    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
    if (!ofmt_ctx) {
        fprintf(stderr, "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }

    stream_mapping_size = ifmt_ctx->nb_streams;
    stream_mapping = (int*)av_mallocz_array(stream_mapping_size, sizeof(*stream_mapping));
    if (!stream_mapping) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    ofmt = ofmt_ctx->oformat;

    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream *out_stream;
        AVStream *in_stream = ifmt_ctx->streams[i];
        AVCodecParameters *in_codecpar = in_stream->codecpar;

        if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
            stream_mapping[i] = -1;
            continue;
        }

        stream_mapping[i] = stream_index++;

        out_stream = avformat_new_stream(ofmt_ctx, NULL);
        if (!out_stream) {
            fprintf(stderr, "Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }

        ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
        if (ret < 0) {
            fprintf(stderr, "Failed to copy codec parameters\n");
            goto end;
        }
        out_stream->codecpar->codec_tag = 0;
    }
    av_dump_format(ofmt_ctx, 0, out_filename, 1);

    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open output file '%s'", out_filename);
            goto end;
        }
    }

    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        goto end;
    }

	int frame_index = 0;

    while (frame_index < totalFrame) {
        AVStream *in_stream, *out_stream;

        ret = av_read_frame(ifmt_ctx, &pkt);
        if (ret < 0)
            break;

        in_stream  = ifmt_ctx->streams[pkt.stream_index];
        if (pkt.stream_index >= stream_mapping_size ||
            stream_mapping[pkt.stream_index] < 0) {
            av_packet_unref(&pkt);
            continue;
        }

		if (pkt.stream_index == AVMEDIA_TYPE_VIDEO) {
			//FIX£ºNo PTS (Example: Raw H.264)
			//Simple Write PTS
			if (pkt.pts == AV_NOPTS_VALUE) {
				//Write PTS
				AVRational time_base1 = in_stream->time_base;
				//Duration between 2 frames (¦Ìs)
				int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(in_stream->r_frame_rate);
				//Parameters
				pkt.pts = (double)(frame_index*calc_duration) / (double)(av_q2d(time_base1)*AV_TIME_BASE);
				pkt.dts = pkt.pts;
				pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1)*AV_TIME_BASE);
				frame_index++;
			}
			//break;
		}

        pkt.stream_index = stream_mapping[pkt.stream_index];
        out_stream = ofmt_ctx->streams[pkt.stream_index];
        log_packet(ifmt_ctx, &pkt, "in");

        /* copy packet */
		AVRounding rnd = (AVRounding)((int)AV_ROUND_NEAR_INF | (int)AV_ROUND_PASS_MINMAX);
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, rnd);
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, rnd);
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
        log_packet(ofmt_ctx, &pkt, "out");

        ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
        if (ret < 0) {
            fprintf(stderr, "Error muxing packet\n");
            break;
        }
        av_packet_unref(&pkt);
    }

    av_write_trailer(ofmt_ctx);

	} while (0);
end:

	if(avio)
	{ 
		av_freep(&avio->buffer);
		av_freep(&avio);
	}
	//iobuffer = NULL;
	
    avformat_close_input(&ifmt_ctx);

    /* close output */
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
        avio_closep(&ofmt_ctx->pb);

    avformat_free_context(ofmt_ctx);

    av_freep(&stream_mapping);

    if (ret < 0 && ret != AVERROR_EOF) {
        //fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        return 1;
    }

	fclose(fp_open);   // only for test

    return 0;
}
