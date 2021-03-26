#include "run.h"

WorKing::WorKing()
    : capture(USB_CAPTURE_DEFULT), cap(ISOPEN_INDUSTRY_CAPTURE) {}

WorKing::~WorKing() {}
/**
 * @brief 运行函数
 * 
 */
void WorKing::Run()
{
#if IS_PARAM_ADJUSTMENT == 1
  namedWindow("parameter", WINDOW_AUTOSIZE);
  createTrackbar("offset_x:", "parameter", &offset_x, 2000, NULL);
  createTrackbar("offset_y:", "parameter", &offset_y, 2000, NULL);
  createTrackbar("_offset_x:", "parameter", &_offset_x, 1, NULL);
  createTrackbar("_offset_y:", "parameter", &_offset_y, 1, NULL);
#endif
  for (;;)
  {
#if FPS_SHOW == 1
    double t = (double)cv::getTickCount(); //开始计时
#endif                                     // #endif
    if (cap.isindustryimgInput())
    {
      frame = cvarrToMat(cap.iplImage, true);
    }
    else
    {
      capture >> frame;
    }

    // 
    Mode_Selection();

#if ISOPEN_INDUSTRY_CAPTURE == 1
    resize(frame, frame, Size(CAMERA_RESOLUTION_COLS, CAMERA_RESOLUTION_ROWS));
#endif
    Mat src_img;
#if ROI_IMG == 1
    // ROI
    if (img.lost_armor_success)
    {
      src_img = frame(img.armor_roi);
      imshow("roi", src_img);
    }
    else
    {
      src_img = frame; //直接赋值
      img.armor_roi = Rect(0, 0, 0, 0);
    }
#elif ROI_IMG == 0
    src_img = frame;
#endif
    // enemy_color = 0;
    // pattern = 0;

    switch (this->pattern)
    {
    case 0://自瞄
        
        img.Pretreat(src_img, enemy_color);
        data_success = img.Processing();
        if(data_success)
        {
            data_type = 1;
            Point2f roi_tl = Point2f(0, 0);
            if(img.lost_armor_success)
            {
                roi_tl = img.armor_roi.tl();
            }
            // if(img.amplitude!=0)
            // {
            //   if(img.amplitude > 0)
            //   {
            //     offset_x = 0;
            //   }
            //   else{
            //     offset_x = 200;
            //   }
            // }
            // cout<<offset_x<<endl;
#if CALL_KALMAN == 1
        data_type = 1;
        Point kalman_point = kalman.predict_point(t,img.armor[img.optimal_armor].armor_rect.center + roi_tl);
        circle(frame, kalman_point, 10, Scalar(0, 0, 255), -1);
        if(kalman_point.x > img.armor[img.optimal_armor].armor_rect.center.x + roi_tl.x)
        {
          offset_x = 800;
        }
        else if(kalman_point.x < img.armor[img.optimal_armor].armor_rect.center.x + roi_tl.x) 
        {
          offset_x = 800;
        }
        else{
          offset_x = 100;
        }

#endif

#if CALL_DEPTH_INFORMATION == 1
        RotatedRect box = RotatedRect(img.armor[img.optimal_armor].armor_rect.center + roi_tl, 
            Size(img.armor[img.optimal_armor].width, img.armor[img.optimal_armor].height), 
            img.armor[img.optimal_armor].tan_angle);
        pnp.vertex_Sort(box);
        rectangle(frame, box.boundingRect(), Scalar(0, 255, 0), 3, 8);
        // RotatedRect kalman_box = RotatedRect(kalman_point, 
        //   Size(img.armor[img.optimal_armor].width, img.armor[img.optimal_armor].height), 
        //   img.armor[img.optimal_armor].tan_angle);
        // rectangle(frame, kalman_box.boundingRect(), Scalar(0, 255, 0), 3, 8);
        // pnp.vertex_Sort(kalman_box);
        
        
        if(img.armor[img.optimal_armor].distinguish == 0)
        {
          pnp.run_SolvePnp(SMALL_ARMORPLATE_WIDTH, ARMORPLATE_HIGHT);
        }
        else
        {
          pnp.run_SolvePnp(BIG_ARMORPLATE_WIDTH, ARMORPLATE_HIGHT);
        }
        yaw = pnp.angle_x;
        pitch = pnp.angle_y;
        depth = int(pnp.dist);
        //补偿角度
        Angle_compensate();
#endif
#if ROI_IMG == 1
        img.Roi_Range();
#endif
      }
      else
      { 
          img.lost_armor_center = Point(0, 0);
          Return_zero();//归零
#if CALL_KALMAN == 1
          kalman.reset();//卡尔曼清零
#endif
      }
      img.Free_memory();
#if CALL_SERIALPORT == 1
      serial.RMserialWrite(_yaw, fabs(yaw)*100, _pitch, fabs(pitch)*100, depth, data_type, is_shooting);
#endif 
      break;

    default://大神符
      
      break;
    }
    imshow("frame", frame);
    cap.cameraReleasebuff();
#if FPS_SHOW == 1
    t = ((double)cv::getTickCount() - t) / cv::getTickFrequency(); //结束计时
    int fps = int(1.0 / t);                                        //转换为帧率
    cout << "FPS: " << fps << endl;                                //输出帧率
#endif
    char c = waitKey(1);
    if (c == 27) //"Esc"-退出
    {
      break;
    }
  }
}
void WorKing::Return_zero()
{
  is_shooting = 0;
  data_type = 0;
  img.armor_success = false;
  img.armor_roi = Rect(0, 0, 0, 0);
  img.lost_armor_success = img.armor_success;
  yaw = 0;
  pitch = 0;
  _yaw = 0;
  _pitch = 0;     
}

/**
 * @brief 选择模式
 *
 */
void WorKing::Mode_Selection()
{
  int ctrl_arr[REC_BUFF_LENGTH];
  serial.RMreceiveData(ctrl_arr);
  enemy_color = ctrl_arr[1];
  pattern = ctrl_arr[2];
  firing = ctrl_arr[3];
}
/**
 * @brief 角度补偿
 * 
 */
void WorKing::Angle_compensate()
{
  if (_offset_x == 0)
  {
    yaw = yaw - offset_x / 100;
  }
  else
  {
    yaw = yaw + offset_x / 100;
  }
  int dist = depth;
  cout<<dist<<endl;
  if(firing == 1)
  {
    offset_y = 0.1616*dist + 96.644;
  }
  else if(firing == 2)
  {
    offset_y = 0.0517+127.69;
  }
  else if(firing == 3)
  {
    if(dist < 2000)
    {
      offset_y = 100;
    }
    else if(dist>=2000)
    {
        switch (dist/1000)
        {
        case 3:
          offset_y = 300;
          break;
        case 4:
          offset_y = 300;
          break;
        case 5:
          offset_y = 400;
          break;
        case 6:
          offset_y = 400;
          break;
        default:
          break;
        }
    }
  }
   
  // if (_offset_y == 0)
  // {
  //   pitch = pitch - offset_y / 100;
  // }
  // else
  // {
  //   pitch = pitch + offset_y / 100;
  // }
  // switch (dist)
  // {
  // case 1:
  //   offset_y = 200;
  //   break;
  // case 2:
  //   offset_y = 400;
  //   break;
  // case 3:
  //   offset_y = 600;
  //   break;
  // case 4:
  //   offset_y = 800;
  //   break;
  // case 5:
  //   offset_y = 750;
  //   break;
  // case 6:
  //   offset_y = 800;
  //   break;
  // default:
  //   break;
  // }
  pitch = pitch - offset_y / 100;

  if (yaw > 0)
  {
    _yaw = 0;
  }
  else
  {
    _yaw = 1;
  }
  if (pitch > 0)
  {
    _pitch = 0;
  }
  else
  {
     _pitch = 1;
  }        
}