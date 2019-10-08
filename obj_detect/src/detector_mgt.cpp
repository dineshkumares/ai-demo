#include "base_os.h"
#include "base_convert.h"
#include "base_string.h"
#include "base_logger.h"
#include "detector_mgt.h"
#include "obj_detector.h"

using namespace std;

Detector_Mgt *g_detector_mgt = NULL;

Detector_Mgt::Detector_Mgt():_cond(_mutex)
{
}


Detector_Mgt::~Detector_Mgt()
{
	std::vector<detector_t*>::iterator iter;
	for (iter = m_detectors.begin(); iter != m_detectors.end(); ++iter)
	{
		if (*iter != NULL)
		{
			if ((*iter)->obj_detector != NULL) delete (*iter)->obj_detector;
			delete (*iter);
		}
	}
}


int Detector_Mgt::init(const std::string &model_dir)
{
	int nRet = 0;

	get_cpu_number_proc(m_detector_size);
	m_free_detector_num = m_detector_size;

	m_detectors.clear();

	for (int i = 0; i < m_detector_size; i++)
	{
		detector_t *detector = new detector_t;
		if (detector == NULL)
		{
			return -1;
		}

		detector->obj_detector  = new ObjDetector(model_dir + "yolov3.weights", model_dir + "yolov3.cfg");
		if (detector->obj_detector == NULL)
		{
			std::cout << "object detector init fail" << std::endl;
			return -1;
		}

		detector->busy = false;

		if (true)
		{
			base::Thread_Mutex_Guard guard(_mutex);
			m_detectors.push_back(detector);
		}
	}

	return nRet;
}

int Detector_Mgt::get_detector(detector_t **detector)
{
	int nRet = 0;

	base::Thread_Mutex_Guard guard(_mutex);

	if (m_free_detector_num == 0)
	{
		//cout << "====wait for free detector========" << endl;
		nRet = _cond.wait(); // wait for free detector
		if (nRet != 0)
		{
			return nRet;
		}
	}

	std::vector<detector_t*>::iterator iter;
	for (iter = m_detectors.begin(); iter != m_detectors.end(); ++iter)
	{
		if (!((*iter)->busy))
		{
			//cout << "found free detector" << endl;
			(*iter)->busy = true;
			*detector = *iter;
			m_free_detector_num--;
			//cout << "free detector num:" << m_free_detector_num << endl;
			return 0;
		}
	}

	return nRet;
}

int Detector_Mgt::put_detector(detector_t **detector)
{
	int nRet = 0;

	base::Thread_Mutex_Guard guard(_mutex);
	if (detector)
	{
		(*detector)->busy = false;
	}
	m_free_detector_num++;
	nRet = _cond.signal();

	return nRet;
}



