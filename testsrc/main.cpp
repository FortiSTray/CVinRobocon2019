#include <iostream>
#include <librealsense2/rs.hpp>
#include "act_d435.h"
#include "robot_locator.h"
#include "kalman_filter.h"
#include "main.h"
#include <thread>


using namespace std;
using namespace std::chrono;

int main(int argc, char* argv[])
{
	std::cout << "[INFO]" << "ActD435 init...\n";
	ActD435			fajD435;
	RobotLocator 	fajLocator;
	kalman_filter   distancefilter;
	double xdistance;
	fajD435.init();
	fajLocator.init(fajD435);
	fajLocator.status = STARTUP_INITIAL;

#ifdef DEBUG
	chrono::steady_clock::time_point start;
	chrono::steady_clock::time_point stop;
	chrono::steady_clock::time_point startt;
	chrono::steady_clock::time_point stopt;
	auto totalTime = chrono::duration_cast<chrono::microseconds>(stopt - startt);
	auto totalTime1 = chrono::duration_cast<chrono::microseconds>(stop - start);
	auto totalTime2 = chrono::duration_cast<chrono::microseconds>(stop - start);
	auto totalTime3 = chrono::duration_cast<chrono::microseconds>(stop - start);
#endif // DEBUG


	distancefilter.initKalmanFilter(2, 1, (cv::Mat_<float>(2, 2) << 1, 0.3, 0, 1), (cv::Mat_<float>(1, 2) << 1, 0), 1e-5, 1e-2, 0.1);
#ifdef __linux__
	std::cout << "[INFO]" << "serial init...\n";
	serial::Serial my_serial("/dev/ttyTHS2", 115200, serial::Timeout::simpleTimeout(2));
	if(my_serial.isOpen())
	{
	   std::cout << "[INFO]" << "serial port initialize ok" << std::endl;
	}else{
	   std::cout << "[ERROR]" << "can't find serial" << std::endl;
	   return -1;
	}
        //get status
        serial::Serial *serialPtr = &my_serial;
        std::cout << "[INFO]" << "GetStatus:\n";
        //GetStatus(serialPtr,&fajLocator.status,&mode);
        
		
    	// openVINO init
    	std::cout << "openvino init\n";
    	TensorRT tensorRT(argc,argv,mode);

    	// camera init
    	std::cout << "camera init\n";
    	// playground-1 only want to be test
    	MvInit mvCamera(0);
#endif
	fajLocator.status = BEFORE_DUNE_STAGE_1;
	thread getCloudPoint(&RobotLocator::updateSrcCloud, &fajLocator);
	thread cloudPointPreprocess(&RobotLocator::cloudPointPreprocess, &fajLocator);
	getCloudPoint.detach();
	cloudPointPreprocess.detach();
	thread getVerticalNormal(&RobotLocator::getVerticalNormal, &fajLocator);
	getVerticalNormal.detach();



	while (!fajLocator.isStoped())
	{
#ifdef DEBUG
		startt = chrono::steady_clock::now();
#endif
		fajLocator.exRoiSegmentStatus = true;
		fajD435.status = fajLocator.status;

		frontDist = 0.0f;
		lateralDist = 0.0f;	
		//steady_clock::time_point t1 = steady_clock::now();
		switch (fajLocator.status)
		{
			case STARTUP_INITIAL:
				fajLocator.status = BEFORE_GRASSLAND_STAGE_1;
				if (fajLocator.status <= BEFORE_GRASSLAND_STAGE_1)
					fajD435.status = BEFORE_GRASSLAND_STAGE_1;
				else
					fajD435.status = fajLocator.status;
				break;

			case BEFORE_DUNE_STAGE_1:
				fajLocator.locateBeforeDuneStage1();
				break;

			case BEFORE_DUNE_STAGE_2:
				fajLocator.locateBeforeDuneStage2();
				break;

			case BEFORE_DUNE_STAGE_3:
				fajLocator.locateBeforeDuneStage3();
				break;

			case PASSING_DUNE:
				fajLocator.locatePassingDune();
				break;

			case BEFORE_GRASSLAND_STAGE_1:
				fajLocator.locateBeforeGrasslandStage1();
				break;

			case BEFORE_GRASSLAND_STAGE_2:
				fajLocator.locateBeforeGrasslandStage2();
				break;

			case UNDER_MOUNTAIN:
				fajLocator.locateUnderMountain();
#ifdef __linux__
				UpdateStatus(serialPtr,&fajLocator.status,&mode);
#endif
				break;

#ifdef __linux__
			case BONE_RECOGNITION:
				std::cout << "start sleep\n";
				sleep(1);
				fajLocator.status = BONE_RECOGNITION_1;
				std::cout << "end sleep to status:" << BONE_RECOGNITION_1 << "\n";
				break;
			case BONE_RECOGNITION_1:
				sleep(1);
				tensorRT.srcImg = mvCamera.getImage();
				//cap >> tensorRT.srcImg;
				//cv::imshow("src",tensorRT.srcImg);
				// inference
				tensorRT.inference();
                                //std::cout << "do\n";//tensorRT.runFlag
                                if(0)
				{
				  fajLocator.status = CLIMBING_MOUNTAIN;
                                  tensorRT.freeTensor();
                                  my_serial.write("go\r\n");
                                }
			
				break;
#endif

			case CLIMBING_MOUNTAIN:
				fajLocator.locateClimbingMountain();
				break;
			case REACH_MOUNTAIN:
				fajLocator.locateReachingMountain();
				break;
			default:
				break;
		}
		//if(status <= 4)
		//	xdistance=distancefilter.predictAndCorrect(Mat_<float>(1, 1)<<fajLocator.diatancemeasurement).at<float>(0,0);
		if (fajLocator.status!=0&& fajLocator.status!=8)
		{
			//SendDatas(serialPtr, dbStatus, frontDist, lateralDist);
		}

#ifdef DEBUG
		stopt = chrono::steady_clock::now();
		totalTime = chrono::duration_cast<chrono::microseconds>(stopt - startt);
		fajLocator.coutMutex.lock();
		cout << "thread 5 " << double(totalTime.count()) / 1000.0f << endl;
		fajLocator.coutMutex.unlock();
		
#endif // DEBUG

		
	}

	return EXIT_SUCCESS;
}
