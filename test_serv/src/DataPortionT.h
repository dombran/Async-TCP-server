#pragma once

#include "stdafx.h"

		// элементарная порция данных для отсылки
		// Преимущества - эффективная передача данных между потоками, поскольку юзает одну и ту же область выделенной памяти
		// без реаллоков, +1/-1 к атомарному счётчику не в счёт
		// содержит опциональный признак маркировки данных
		template<class MarkType>
		class DataPortionT
		{
			// опциональная маркировка данной порции данных
			boost::optional<MarkType>		m_optMark;

		public:
			boost::shared_array<uint8_t>	m_pBuff;

			// длинна данных
			size_t							m_Len;

			DataPortionT() : m_Len()
			{}

			// инциализация блоком данных
			// выделяет блок данных заданного размера и копирует туда указанный блок данных.
			// если pData==nullptr то происходит просто выделение памяти нужного размера
			DataPortionT(const void* pData, size_t Len, boost::optional<MarkType> optMark=boost::none): m_optMark(optMark)
			{
				Init(pData, Len);
			}

			DataPortionT(const DataPortionT& an) = default;

			DataPortionT(DataPortionT&& an) : m_Len()
			{
				*this = std::move(an);
			}

			DataPortionT& operator=(DataPortionT&& an)
			{
				std::swap(m_pBuff, an.m_pBuff);
				std::swap(m_Len, an.m_Len);
				std::swap(m_optMark, an.m_optMark);

				return *this;
			}

			DataPortionT& operator=(const DataPortionT& an) = default;


			// инциализация блоком данных
			// выделяет блок данных заданного размера и копирует туда указанный блок данных.
			// если pData==nullptr то происходит просто выделение памяти нужного размера
			void Init(const void* pData, size_t Len)
			{
				m_pBuff.reset(new uint8_t[Len]);

				// специальный кул-хак с нулевым указателем. тупо выделение памяти. без копирования
				if (pData)
					memcpy(m_pBuff.get(), pData, Len);

				m_Len = Len;
			}

			// это для удобства запихивания в boost async_send
			auto GetBuffer(size_t offset = 0) const
			{
				return boost::asio::buffer(m_pBuff.get() + offset, m_Len - offset);
			}

			// поиметь признак маркировки
			boost::optional<MarkType> GetOptMark() const
			{
				return m_optMark;
			}
		};

		// специализация под тип boost::blank
		// Борис. Строго говоря я хочу пустой тип наподобие void, но с ним нужен SFINAE для реализации всяких TcpClasses
		// а мне влом разбираться с SFINAE.
		// короче это тип данных где ВООБЩЕ НЕ ПЛАНИРУЕТСЯ МАРКИРОВКА!
		// для него функция извлечения признака маркировки всегда возвращает её отсутствие
		template<>
		class DataPortionT<boost::blank>
		{
		public:
			boost::shared_array<uint8_t>	m_pBuff;

			// длинна данных
			size_t							m_Len;

			DataPortionT() : m_Len()
			{}

			// инциализация блоком данных
			// выделяет блок данных заданного размера и копирует туда указанный блок данных.
			// если pData==nullptr то происходит просто выделение памяти нужного размера
			DataPortionT(const void* pData, size_t Len, boost::optional<boost::blank> optMark=boost::none)
			{
				Init(pData, Len);
			}

			DataPortionT(const DataPortionT& an) = default;

			DataPortionT(DataPortionT&& an) : m_Len()
			{
				*this = std::move(an);
			}

			DataPortionT& operator=(DataPortionT&& an)
			{
				std::swap(m_pBuff, an.m_pBuff);
				std::swap(m_Len, an.m_Len);

				return *this;
			}

			DataPortionT& operator=(const DataPortionT& an) = default;


			// инциализация блоком данных
			// выделяет блок данных заданного размера и копирует туда указанный блок данных.
			// если pData==nullptr то происходит просто выделение памяти нужного размера
			void Init(const void* pData, size_t Len)
			{
				m_pBuff.reset(new uint8_t[Len]);

				// специальный кул-хак с нулевым указателем. тупо выделение памяти. без копирования
				if (pData)
					memcpy(m_pBuff.get(), pData, Len);

				m_Len = Len;
			}

			// это для удобства запихивания в boost async_send
			auto GetBuffer(size_t offset = 0) const
			{
				return boost::asio::buffer(m_pBuff.get() + offset, m_Len - offset);
			}

			// поиметь признак маркировки
			boost::optional<boost::blank> GetOptMark() const
			{
				return boost::none;
			}
		};



