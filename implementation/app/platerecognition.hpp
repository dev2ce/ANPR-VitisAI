/*
 * Copyright 2020 Avnet Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once
#include <vitis/ai/demo.hpp>
#include <vitis/ai/platedetect.hpp>
#include <vitis/ai/platenum.hpp>
#include <vitis/ai/ssd.hpp>
#include <iostream>
#include <vitis/ai/yolov3.hpp>
#include <chrono>

#include <fstream>
#include <sstream>

using namespace std;
namespace vitis {
namespace ai {

struct PlateRecognition {
  static std::unique_ptr<PlateRecognition> create();
  PlateRecognition();
  std::vector<vitis::ai::YOLOv3Result> run(const cv::Mat &input_image);
  int getInputWidth();
  int getInputHeight();
  size_t get_input_batch();

private:
  std::unique_ptr<vitis::ai::YOLOv3> car_detect_;
  std::unique_ptr<vitis::ai::YOLOv3> plate_detect_;
  std::unique_ptr<vitis::ai::YOLOv3> plate_num_;

  bool debug;
};

std::unique_ptr<PlateRecognition> PlateRecognition::create() {
  return std::unique_ptr<PlateRecognition>(new PlateRecognition());
}
int PlateRecognition::getInputWidth() { return car_detect_->getInputWidth(); }
int PlateRecognition::getInputHeight() { return car_detect_->getInputHeight(); }
size_t PlateRecognition::get_input_batch() { return car_detect_->get_input_batch(); }

PlateRecognition::PlateRecognition():
      car_detect_{vitis::ai::YOLOv3::create("dpu_yolov3-tiny_3l-vehicle_608_v3")},
      plate_detect_{vitis::ai::YOLOv3::create("dpu_yolov3-tiny_3l-plate_608_v4")}, 
      plate_num_{vitis::ai::YOLOv3::create("dpu_yolov3-tiny_3l-char_608_v3")} 
{
  const char * val = std::getenv( "PLATERECOGNITION_DEBUG" );
  if ( val == nullptr ) {
    debug = true;
  }
  else {
    cout << "[INFO] PLATERECOGNITION_DEBUG" << endl;
    debug = true;
  }
}


//std::vector<vitis::ai::PlateNumResult>
std::vector<vitis::ai::YOLOv3Result>
PlateRecognition::run(const cv::Mat &input_image) {
  std::vector<vitis::ai::YOLOv3Result> mt_results;
  cv::Mat image;
  image = input_image;
  string char_labels[31] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F", "G", "H", "K", "L", "M", "N", "P", "R", "S", "T", "U", "V", "X", "Y", "Z"};
  static int frame_number = 0;
  frame_number++;
  if ( debug == true ) {
    cout << "Frame " << frame_number << endl;
  }

  float model1_time = 0.0;
  float model2_time = 0.0;
  float model3_time = 0.0;
  float total_time = 0.0;
  // run vehicle detection
  auto t_start = chrono::high_resolution_clock::now();
  auto car_detect_results = car_detect_->run(image);
  auto t_model_1_stop = chrono::high_resolution_clock::now();
  model1_time = (float) chrono::duration_cast<chrono::milliseconds>(t_model_1_stop - t_start).count();
  cout << "MODEL 1:" << model1_time << " ms" << endl;
  cout << "NUM OF VEHICLES: " << car_detect_results.bboxes.size() << endl;
  for ( const auto bbox : car_detect_results.bboxes ) {
    //if ( bbox.label == 1) continue; 
    if ( bbox.score < 0.50 ) continue; 
    string rec_result = "";
    string plate_type = "";
    string label;
    cv::Scalar color;
    switch (bbox.label) {
      case 0:  
        label = "car";    
	color = cv::Scalar(0,255,0);
	break;
      case 2:  
	label = "truck"; 
	color = cv::Scalar(0,0,255);
	break;
      case 1:  
	label = "bus"; 
	color = cv::Scalar(255,0,0);
	break;
      default: 
	label = "unknown";    
	color = cv::Scalar(255,255,255);
	break;	      
    }	     

    auto bbox_roi = cv::Rect{
      (int)(bbox.x * image.cols),
      (int)(bbox.y * image.rows),
      (int)(bbox.width  * image.cols),
      (int)(bbox.height * image.rows)
      };

    // only consider results with confidence of 50% or more
    //if ( bbox.score < 0.50 ) continue; 
    
    if(bbox_roi.x <= 0) bbox_roi.x = 1;
    if(bbox_roi.y <= 0) bbox_roi.y = 1;
    if(bbox_roi.x + bbox_roi.width >= image.cols) bbox_roi.width = image.cols - bbox_roi.x - 1;
    if(bbox_roi.y + bbox_roi.height >= image.rows) bbox_roi.height = image.rows - bbox_roi.y - 1;

    cv::rectangle(image, bbox_roi, color);
    cv::putText(image,label,cv::Point(bbox_roi.x,bbox_roi.y),cv::FONT_HERSHEY_SIMPLEX,0.5,color,2);
    if ( debug == true ) {
      cout << "  VehicleDetect : label=" << label << " x,y,w,h=" << bbox_roi.x << "," << bbox_roi.y << "," << bbox_roi.width << "," << bbox_roi.height << " confidence=" << bbox.score << endl;
    }

    // only look for license plates on vehicles
    //if ( bbox.label == 1 ) continue; 
    if (0 <= bbox_roi.x && 0 <= bbox_roi.width && bbox_roi.x + bbox_roi.width <= image.cols && 0 <= bbox_roi.y && 0 <= bbox_roi.height && bbox_roi.y + bbox_roi.height <= image.rows){
	    cv::Mat bbox_img = image(bbox_roi);
	    // run plate_detect
	    auto t_model_2 = chrono::high_resolution_clock::now();
	    auto plate_detect_result = plate_detect_->run(bbox_img);
	    auto t_model_2_stop = chrono::high_resolution_clock::now();
	    model2_time = (float) chrono::duration_cast<chrono::milliseconds>(t_model_2_stop - t_model_2).count();
	    cout << "    MODEL 2:" << model2_time << " ms" << endl;
	    // if more than 1 plate for a vehicle then skip
	    if (plate_detect_result.bboxes.size() == 0) continue;
	    if (plate_detect_result.bboxes.size() > 1)	{
	    	for(int i = 0; i < plate_detect_result.bboxes.size(); i++){
	    		for(int j = 0; j < plate_detect_result.bboxes.size()-1; j++){
	    			if(plate_detect_result.bboxes[j].score < plate_detect_result.bboxes[j+1].score){
	    				auto temp = plate_detect_result.bboxes[j];
	    				plate_detect_result.bboxes[j] = plate_detect_result.bboxes[j+1];
	    				plate_detect_result.bboxes[j+1] = temp;
	    			}
	    		}
	    	}
	    }
	    string plate_label; 
	    auto plate_bbox = plate_detect_result.bboxes[0];
	    switch (plate_bbox.label) {
	      case 0:  
		plate_label = "single-line";    
		color = cv::Scalar(0,255,0);
		break;
	      case 1:  
		plate_label = "double-line"; 
		color = cv::Scalar(0,0,255);
		break;
	      default: 
		label = "unknown";    
		color = cv::Scalar(255,255,255);
		break;	      
	    }
	    plate_type = plate_label;
	    auto roi = cv::Rect{
	      (int)(plate_bbox.x * bbox_img.cols),
	      (int)(plate_bbox.y * bbox_img.rows),
	      (int)(plate_bbox.width  * bbox_img.cols),
	      (int)(plate_bbox.height * bbox_img.rows)
	      };  
	      
	    // only consider platedetect results with confidence of 80% or more
	    //if ( plate_bbox.score < 0.70 ) continue; 

	    cv::rectangle(bbox_img, roi, cv::Scalar(0, 0, 255));

	    if ( debug == true ) {
	      cout << "    PlateDetect : x,y,w,h=" << roi.x << "," << roi.y << "," << roi.width << "," << roi.height << " label=" << plate_label <<" confidence=" << plate_bbox.score << endl;
	    }
	    
	    if (0 <= roi.x && 0 <= roi.width && roi.x + roi.width <= bbox_img.cols && 0 <= roi.y && 0 <= roi.height && roi.y + roi.height <= bbox_img.rows) {

		cv::Mat sub_img = bbox_img(roi);

		// run plate_num
		auto t_model_3 = chrono::high_resolution_clock::now();
		auto plate_num_result = plate_num_->run(sub_img);
		auto t_model_3_stop = chrono::high_resolution_clock::now();
		model3_time = (float) chrono::duration_cast<chrono::milliseconds>(t_model_3_stop - t_model_3).count();
		cout << "      MODEL 3:" << model3_time << " ms" << endl;
		if (plate_num_result.bboxes.size() < 1) continue;
		
		std::vector<vitis::ai::YOLOv3Result::BoundingBox> char_result;
		
		for ( const auto char_bbox : plate_num_result.bboxes )  {
			if ( char_bbox.score < 0.2 ) continue;
			char_result.push_back(char_bbox);
			auto char_roi = cv::Rect{
				(int)(char_bbox.x * sub_img.cols),
				(int)(char_bbox.y * sub_img.rows),
				(int)(char_bbox.width  * sub_img.cols),
				(int)(char_bbox.height * sub_img.rows)
			};
			//if ( char_bbox.score < 0.50 ) continue;
			cv::rectangle(sub_img, char_roi, cv::Scalar(255, 0, 255));
			cout <<"      CharDetect: " << char_labels[char_bbox.label] << " confidence=" << char_bbox.score << " x: " << char_bbox.x << " y: "<< char_bbox.y << endl;			
		}
		
		if(plate_label == "single-line"){
			auto recog_results = char_result;
			vitis::ai::YOLOv3Result::BoundingBox temp;
			for(int i = 0; i < recog_results.size(); i++){
				for(int j = 0; j < recog_results.size()-1; j++){
					if(recog_results[j].x > recog_results[j+1].x){
						temp = recog_results[j];
						recog_results[j] = recog_results[j+1];
						recog_results[j+1] = temp;
					}
				}
			}
			
			if(recog_results.size() > 1){
				for(int i = 0; i < recog_results.size()-1; i++){
					if((recog_results[i+1].x - recog_results[i].x) < 0.02){
						if(recog_results[i+1].score > recog_results[i].score)
							recog_results.erase(recog_results.begin() + i);
						else
							recog_results.erase(recog_results.begin() + i + 1);
						i = i-1;
					}
				} 
				
				if(recog_results.size() > 2){
					if(recog_results[2].label == 3)  recog_results[2].label = 14;
					if(recog_results[2].label == 8)  recog_results[2].label = 11;
					if(recog_results[2].label == 6)  recog_results[2].label = 16;
					if(recog_results[2].label == 5)  recog_results[2].label = 24;
					if(recog_results[2].label == 2)  recog_results[2].label = 30;
					if(recog_results[2].label == 0)  recog_results[2].label = 13;
					if(recog_results[2].label == 1)  recog_results[2].label = 15;
					if(recog_results[2].label == 4)  recog_results[2].label = 10;
				} 
			}
			
			if(recog_results.size() > 6 && recog_results.size() < 10)
				for(int i = 0; i < recog_results.size(); i++){
					rec_result += char_labels[recog_results[i].label];
				}
		} else {
			auto recog_results = char_result;
			std::vector<vitis::ai::YOLOv3Result::BoundingBox> first_line;
			std::vector<vitis::ai::YOLOv3Result::BoundingBox> second_line;
			std::vector<vitis::ai::YOLOv3Result::BoundingBox> final_result;
			for(int i = 0; i < recog_results.size(); i++){
				if(recog_results[i].y < 0.3)
					first_line.push_back(recog_results[i]);
				else
					second_line.push_back(recog_results[i]);
			}
			
			vitis::ai::YOLOv3Result::BoundingBox temp;
			if(first_line.size() > 1)
				for(int i = 0; i < first_line.size(); i++){
					for(int j = 0; j < first_line.size()-1; j++){
						if(first_line[j].x > first_line[j+1].x){
							temp = first_line[j];
							first_line[j] = first_line[j+1];
							first_line[j+1] = temp;
						}
					}
					
					
				}
			
				
			if(first_line.size() > 1){
				for(int i = 0; i < first_line.size()-1; i++){
					if((first_line[i+1].x - first_line[i].x) < 0.02){
						if(first_line[i+1].score > first_line[i].score)
							first_line.erase(first_line.begin() + i);
						else
							first_line.erase(first_line.begin() + i + 1);
						i = i-1;
					}
				} 
				
				if(first_line.size() > 2){
					if(first_line[2].label == 3)  first_line[2].label = 14;
					if(first_line[2].label == 8)  first_line[2].label = 11;
					if(first_line[2].label == 6)  first_line[2].label = 16;
					if(first_line[2].label == 5)  first_line[2].label = 24;
					if(first_line[2].label == 2)  first_line[2].label = 30;
					if(first_line[2].label == 0)  first_line[2].label = 13;
					if(first_line[2].label == 1)  first_line[2].label = 15;
					if(first_line[2].label == 4)  first_line[2].label = 10;
				} 
			}	
			
			if(second_line.size() > 1)
				for(int i = 0; i < second_line.size(); i++){
					for(int j = 0; j < second_line.size()-1; j++){
						if(second_line[j].x > second_line[j+1].x){
							temp = second_line[j];
							second_line[j] = second_line[j+1];
							second_line[j+1] = temp;
						}
					}
				}			
			
			if(second_line.size() > 1){
				for(int i = 0; i < second_line.size()-1; i++){
					if((second_line[i+1].x - second_line[i].x) < 0.02){
						if(second_line[i+1].score > second_line[i].score)
							second_line.erase(second_line.begin() + i);
						else
							second_line.erase(second_line.begin() + i + 1);
						i = i-1;
					}
				} 
			}		
				
			if(first_line.size() > 0)
				for(int i = 0; i < first_line.size(); i++){
					final_result.push_back(first_line[i]);
				}
			if(second_line.size() > 0)	
				for(int i = 0; i < second_line.size(); i++){
					final_result.push_back(second_line[i]);
				}
				
			if(final_result.size() > 6 && final_result.size() < 10)	{			
				for(int i = 0; i < final_result.size(); i++){
					rec_result += char_labels[final_result[i].label];
				}
			}
		} 

		mt_results.emplace_back(plate_num_result);
	    } 
	    
    }
    cout << "  Result: " << rec_result << endl;
    //cout << "  Plate type " << plate_type << endl; 
    cv::rectangle(image, cv::Point(bbox_roi.x, bbox_roi.y + bbox_roi.height), cv::Point(bbox_roi.x + bbox_roi.width, bbox_roi.y + bbox_roi.height - 30), cv::Scalar(255, 255, 255), cv::FILLED);
    cv::putText(image, rec_result ,cv::Point(bbox_roi.x, bbox_roi.y + bbox_roi.height),cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0,0,255), 2, false);
  }
  auto t_stop = chrono::high_resolution_clock::now();
  total_time = (float) chrono::duration_cast<chrono::milliseconds>(t_stop - t_start).count();
  float FPS = 1000/total_time;
  cout << "Total:" << total_time << " ms" << endl;
  cout << "FPS:" << FPS << endl;
  string topText = cv::format("Total time: %.2f - FPS : %.2f ", total_time, FPS);
  putText(image, topText, cv::Point(30, 30), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0,255,0), 2, false);
  return mt_results;
}
      
} // namespace ai
} // namespace vitis
