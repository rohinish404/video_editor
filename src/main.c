#include "raylib.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/time.h>

#define MAX_QUEUE_SIZE 30

typedef struct {
  AVFrame *frames[MAX_QUEUE_SIZE];
  size_t read_index;
  size_t write_index;
  size_t size;
} FrameQueue;

void init_queue(FrameQueue *frameQueue) {
  for (size_t i = 0; i < MAX_QUEUE_SIZE; ++i) {
    frameQueue->frames[i] = av_frame_alloc();
  }
  frameQueue->write_index = 0;
  frameQueue->read_index = 0;
  frameQueue->size = 0;
}

int push_queue(FrameQueue *queue, AVFrame *src) {
  if (queue->size >= MAX_QUEUE_SIZE) return -1;
  av_frame_unref(queue->frames[queue->write_index]);
  av_frame_ref(queue->frames[queue->write_index], src);
  queue->write_index = (queue->write_index + 1) % MAX_QUEUE_SIZE;
  queue->size++;
  return 0;
}

AVFrame *pop_queue(FrameQueue *q) {
  if (q->size <= 0) return NULL;
  AVFrame* frame = q->frames[q->read_index];
  q->read_index = (q->read_index + 1) % MAX_QUEUE_SIZE;
  q->size--;
  return frame;
}

int decode_video(AVCodecContext *avctx, AVFrame *frame, int *got_frame, AVPacket *pkt) {
  int ret;

  *got_frame = 0;

  if (pkt) {
    ret = avcodec_send_packet(avctx, pkt);
    if (ret < 0) {
      return ret;
    }
  }

  ret = avcodec_receive_frame(avctx, frame);
  if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
    return ret;
  }
  if (ret >= 0) {
    *got_frame = 1;
  }

  return 0;
}

int main(void) {
  const int screenWidth = 800;
  const int screenHeight = 450;

  InitWindow(screenWidth, screenHeight, "Video Player");

  AVFormatContext *formatContext = NULL;
  if (avformat_open_input(&formatContext, "assets/sunset.mp4", NULL, NULL) != 0) {
    printf("Couldn't open video file\n");
    return -1;
  }

  if (avformat_find_stream_info(formatContext, NULL) < 0) {
    printf("Couldn't find stream information\n");
    return -1;
  }

  int videoStream = -1;
  for (int i = 0; i < formatContext->nb_streams; i++) {
    if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      videoStream = i;
      break;
    }
  }
  if (videoStream == -1) {
    printf("Couldn't find a video stream\n");
    return -1;
  }

  AVCodecContext *codecContext = avcodec_alloc_context3(NULL);
  if (!codecContext) {
    fprintf(stderr, "Failed to allocate codec context\n");
    return -1;
  }

  if (avcodec_parameters_to_context(codecContext, formatContext->streams[videoStream]->codecpar) < 0) {
    fprintf(stderr, "Failed to copy codec parameters to codec context\n");
    return -1;
  }
  int duration = formatContext->streams[videoStream]->duration;
  printf("duration %d\n",duration);
  int pts;
  const AVCodec *codec = avcodec_find_decoder(codecContext->codec_id);
  if (codec == NULL) {
    printf("Unsupported codec\n");
    return -1;
  }

  if (avcodec_open2(codecContext, codec, NULL) < 0) {
    printf("Couldn't open codec\n");
    return -1;
  }

  FrameQueue frameQueue;
  init_queue(&frameQueue);

  AVFrame *frame = av_frame_alloc();
  AVPacket *packet = av_packet_alloc();

  Texture2D texture = LoadTextureFromImage(GenImageColor(codecContext->width, codecContext->height, BLACK));

  SetTargetFPS(30); 
  while (!WindowShouldClose()) {
    while (frameQueue.size < MAX_QUEUE_SIZE && av_read_frame(formatContext, packet) >= 0) {
      if (packet->stream_index == videoStream) {
        int got_frame = 0;
        int ret = decode_video(codecContext, frame, &got_frame, packet);

        if (ret < 0) {
          printf("Error decoding video\n");
          break;
        }
        if (got_frame) {
          push_queue(&frameQueue, frame);
        }
      }
      av_packet_unref(packet);
    }

    if (frameQueue.size > 0) {
      AVFrame *next_frame = pop_queue(&frameQueue);
      pts = next_frame->pts;
      struct SwsContext *sws_ctx = sws_getContext(
        codecContext->width, codecContext->height, codecContext->pix_fmt,
        codecContext->width, codecContext->height, AV_PIX_FMT_RGBA,
        SWS_BILINEAR, NULL, NULL, NULL
      );

      uint8_t *pixels = malloc(codecContext->width * codecContext->height * 4);
      uint8_t *dest[4] = {pixels, NULL, NULL, NULL};
      int dest_linesize[4] = {codecContext->width * 4, 0, 0, 0};

      sws_scale(sws_ctx, (const uint8_t * const*)next_frame->data,
                next_frame->linesize, 0, codecContext->height, dest, dest_linesize);

      UpdateTexture(texture, pixels);
      free(pixels);

      sws_freeContext(sws_ctx);
      av_frame_unref(next_frame);
    }

    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawTexture(texture, 0, 0, WHITE);
    DrawRectangle(10, 390, duration/duration*(screenWidth-50),50,BLACK);
    printf("pts %f\n",(float)(((float)pts/(float)duration)+10));
    DrawRectangle((float)((((float)pts/(float)duration)*750)+10),390,10,50,RED);
    EndDrawing();
  }

  UnloadTexture(texture);
  av_frame_free(&frame);
  av_packet_free(&packet);
  for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
    av_frame_free(&frameQueue.frames[i]);
  }
  avcodec_free_context(&codecContext);
  avformat_close_input(&formatContext);

  CloseWindow();

  return 0;
}
