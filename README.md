# 3rd-PW-assignment

Zadanie polega na wykonaniu kilku implementacji kolejki nie-blokującej z wieloma czytelnikami i pisarzami: SimpleQueue, RingsQueue, LLQueue, BLQueue. Dwie implementacje polegają na użyciu zwykłych mutexów, a dwie kolejne na użyciu atomowych operacji, w tym kluczowego compare_exchange.

W każdym przypadku implementacja składa się ze struktury <kolejka> oraz metod:

<kolejka>* <kolejka>_new(void) - alokuje (malloc) i inicjalizuję nową kolejkę.

void <kolejka>_delete(<kolejka>* queue) - zwalnia wszelką pamięć zaalokowaną przez metody kolejki.

void <kolejka>_push(<kolejka>* queue, Value value) - dodaje wartość na koniec kolejki.

Value <kolejka>_pop(<kolejka>* queue) - pobiera wartość z początku kolejki albo zwraca EMPTY_VALUE jeśli kolejka jest pusta.

bool <kolejka>_is_empty(<kolejka>* queue) - sprawdza czy kolejka jest pusta.

(Przykładowo pierwsza implementacja powinna definiować strukturę SimpleQueue oraz metody SimpleQueue* SimpleQueue_new(void) itp.)

--------


Użytkownicy kolejki nie używają wartości EMPTY_VALUE=0 ani TAKEN_VALUE=-1 (można je wykorzystać jako specjalne).

Użytkownicy kolejki gwarantują, że wykonują new/delete dokładnie raz, odpowiednio przed/po wszystkich operacjach push/pop/is_empty ze wszystkich wątków.



------------
SimpleQueue: kolejka z listy jednokierunkowej, z dwoma mutexami

To jedna z prostszych implementacji kolejki. Osobny mutex dla producentów i dla konsumentów pozwala lepiej zrównoleglić operacje.

--------------
RingsQueue: kolejka z listy buforów cyklicznych

To połączenie SimpleQueue z kolejką zaimplementowaną na buforze cyklicznym, łączące nieograniczony rozmiar pierwszej z wydajnością drugiej (listy jednokierunkowe są stosunkowo powolne ze względu na ciągłe alokacje pamięci).

-------------
LLQueue: kolejka lock-free z listy jednokierunkowej

To jedna z najprostszych implementacji kolejki lock-free.

--------------
BLQueue: kolejka lock-free z listy buforów

To jedna z prostszych a bardzo wydajnych implementacji kolejki. Idea tej kolejki to połączenie rozwiązania z listą jednokierunkową z rozwiązaniem gdzie kolejka jest prostą tablicą z atomowymi indeksami do wstawiania i pobierania elementów (ale liczba operacji byłaby ograniczona przez długość tablicy). Łączymy zalety obu robiąc listę tablic; do nowego węzła listy potrzebujemy przejść dopiero kiedy tablica się zapełniła. Tablica nie jest jednak tutaj buforem cyklicznym, każde pole w niej jest zapełnione co najwyżej raz (wariant z buforami cyklicznymi byłby dużo trudniejszy).

----------------------------
Hazard Pointer

Hazar Pointer to technika do radzenia sobie z problemem bezpiecznego zwalniania pamięci w strukturach danych współdzielonych przez wiele wątków oraz z problemem ABA. Idea jest taka, że każdy wątek może zarezerwować sobie jeden adres na węzeł (jeden na każdą instację kolejki), który potrzebuje zabezpieczyć przed usunięciem (lub podmianą ABA) na czas operacji push/pop/is_empty. Chcąc zwolnić węzeł (free()), wątek zamiast tego dodaje jego adres na swój zbiór wycofanych adresów i później okresowo przegląda ten zbiór, zwalniając adresy, które nie są zarezerwowane.
