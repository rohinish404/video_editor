#include "raylib.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>


int decode_video(AVCodecContext *avctx, AVFrame *frame, int *got_frame, AVPacket *pkt)
{
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

int main(void){

  const int screenWidth = 800;
  const int screenHeight = 450;

  InitWindow(screenWidth, screenHeight,"video editor");
  AVFormatContext *formatContext = NULL;
  if (avformat_open_input(&formatContext, "assets/sunset.mp4", NULL, NULL) != 0) {
    printf("Couldn't open video file\n");
    return -1;
  }

  int videoStream = -1;
  for (int i = 0; i < formatContext->nb_streams; i++) {
    if (formatContext->streams[i]->codecpar->codec_type== AVMEDIA_TYPE_VIDEO) {
      videoStream = i;
      printf("%d",videoStream);
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
  const AVCodec *codec = avcodec_find_decoder(codecContext->codec_id);
  if (codec == NULL) {
    printf("Unsupported codec\n");
    return -1;
  }
  if (avcodec_open2(codecContext, codec, NULL) < 0) {
    printf("Couldn't open codec\n");
    return -1;
  }
  AVFrame *frame = av_frame_alloc();
  AVPacket packet;

  Image image = GenImageColor(codecContext->width, codecContext->height, BLACK);
  Texture2D texture = LoadTextureFromImage(image);

  SetTargetFPS(60);

  while (!WindowShouldClose())
  {
    if (av_read_frame(formatContext, &packet) >= 0) {
      if (packet.stream_index == videoStream) {
        int got_frame = 0;
        int ret = decode_video(codecContext, frame, &got_frame, &packet);

        if (ret < 0) {
          printf("Error decoding video\n");
          break;
        }

        if (got_frame) {

          struct SwsContext *sws_ctx = sws_getContext(
            codecContext->width, codecContext->height, codecContext->pix_fmt,
            codecContext->width, codecContext->height, AV_PIX_FMT_RGB24,
            SWS_BILINEAR, NULL, NULL, NULL
          );
          uint8_t *buffer = (uint8_t *)malloc(codecContext->width * codecContext->height * 3);
          int linesize[1] = { 3 * codecContext->width };

          sws_scale(sws_ctx, (const uint8_t * const*)frame->data, frame->linesize, 0,
                    codecContext->height, &buffer, linesize);
          for (int y = 0; y < codecContext->height; y++) {
            for (int x = 0; x < codecContext->width; x++) {
              Color color = {
                buffer[(y * codecContext->width + x) * 3],
                buffer[(y * codecContext->width + x) * 3 + 1],
                buffer[(y * codecContext->width + x) * 3 + 2],
                255
              };
              ImageDrawPixel(&image, x, y, color);
            }
          }

          UpdateTexture(texture, image.data);
          free(buffer);
          sws_freeContext(sws_ctx);
        }
      }
      av_packet_unref(&packet);
    }
    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawTexture(texture, 0, 0, WHITE);
    EndDrawing();


  }
  UnloadTexture(texture);
  UnloadImage(image);
  av_frame_free(&frame);
  avcodec_free_context(&codecContext); 
  avformat_close_input(&formatContext);
  CloseWindow();

  return 0;



}
