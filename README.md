# ThreadAndRequest
  
 Демонстрационная программа по многопоточности и работы с очередью на C++ с WinAPI.
 
 Программа создаёт потоки которые симулируют создание и обработку запросов. Количество потоков "создающих" и обрабатывающих регулируется отдельно.
 Время работы программы устанавливается в секундах. Разброс простоев для симуляции выполнения процесса устанавливается в миллисекундах.
 Так же можно установить сколько максимально, от времени работы программы, будут генерироваться запросы (1 всё время, 0,7 - 70% и тд).
 
 TODO:
 Откорректировать работу с выводом сообщений о протекании процесса работы у каждого потока.
