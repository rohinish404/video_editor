#include "raylib.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
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
    //if (av_read_frame(formatContext, &packet) >= 0) {
    //        if (packet.stream_index == videoStream) {
    //            // Decode video frame
    //            int frameFinished;
    //            avcodec_decode_video2(codecContext, frame, &frameFinished, &packet);

    //            if (frameFinished) {
    //                // Convert the image from its native format to RGB
    //                struct SwsContext *sws_ctx = sws_getContext(
    //                    codecContext->width, codecContext->height, codecContext->pix_fmt,
    //                    codecContext->width, codecContext->height, AV_PIX_FMT_RGB24,
    //                    SWS_BILINEAR, NULL, NULL, NULL
    //                );

    //                // Update the image data
    //                UpdateImageRaw(&image, frame->data[0]);

    //                // Update the texture
    //                UpdateTexture(texture, image.data);

    //                sws_freeContext(sws_ctx);
    //            }
    //        }
    //        av_packet_unref(&packet);
    //    }
      BeginDrawing();
        ClearBackground(RAYWHITE);
       // DrawTexture(texture, 0, 0, WHITE);
      DrawCircle(screenWidth/5, 120,30, DARKBLUE);

    EndDrawing();


  }
  UnloadTexture(texture);
  UnloadImage(image);
  av_frame_free(&frame);
  avcodec_close(codecContext);
  avformat_close_input(&formatContext);
  CloseWindow();
  
  return 0;



}
