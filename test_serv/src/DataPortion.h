﻿#pragma once

#include "DataPortionT.h"


		// элементарная порция данных для отсылки
		// Преимущества - эффективная передача данных между потоками, поскольку юзает одну и ту же область выделенной памяти
		// без реаллоков, +1/-1 к атомарному счётчику не в счёт
		typedef DataPortionT<boost::blank> DataPortion;