RENA CANGA 		1115201700218

DiseaseAggregator
Communication Protocol : 
	Στο τέλος κάθε μηνύματος ο writer γράφει &, το οποίο λέει στον reader ότι το μήνυμα τελείωσε και αυτός σπάει τη λούπα.
	Οταν ο worker δεν έχει την απάντηση του ερωτήματος του χρήστη ή υπάρχει κάποιο λάθος γράφει στο pipe του Error και ο parent τον 	αγνοεί.
	Τα μηνύματα πρώτα συνθέτονται από τους workers σε ένα string και μετά μεταφέρονται στον parent μέσο pipe.

Directories distribution : 
	Αν τα Directories είναι περισσότερα από τους workers μειράζονται με Round-Robin fashion. Αν τα Directories δεν αρκούν για όλους, 		ο γονιός θα στείλει ένα μήνύμα(noDir) το οποίο θα τερματλησει τους workers αυτους και στη συνέχεια θα κλήσει τα pipes.

Statistics :
	Για κάθε αρχείο στα Directories, που έχει ανατεθεί στους workers, φτιαχνεί ένα Hashtable με βάση την ασθένεια άπο όπου μετράει 		τους ασθενείς ανά ηλικία, συνθέτει το μήνυμα και το στέλνει μέσο pipe. Διαγράφει το Hashtable όταν τελειώνει ένα αρχείο.

Database Structures : 
	List -> κρατάει όλες τις εγγραφές των ασθενών. Πριν την πρόσθεση ενός EXIT ελέγχει αν το id υπάρχει, αλλιώς εκτυπώνει λάθος.
		Αν υπάρχει ενημερώμει το record με το exitdate.
	Hashtable -> (by Country και by Disease) Κάθε bucket έχει μια χώρα/ασθένεια και ένα δείκτη σε RBTree, το οποίο έχει δείκτες
		στους ασθενείς στη λίστα. Το RBTree τα αποθηκεύει με βάση το id.

Signals : 
	Υπάρχει ένας signal handler για τον parent και ένας για κάθε worker, ο οποίος διαχειρίζεται τα σήματα που του πιάνει.
	Αν ο worker πάρει ένα SIGINT/SIGQUIT θα φτιάξει ένα logfile και θα τερματίσει.
	Αν ο worker πάρει ένα SIGUSR1 αλλάζει την τιμή ενός global flag, το οποίο ελέγχει το condition του update Structures. Ο χρήστης 
	πρέπει να δώσει ENTER για να προχωρίσει η διαδικασία.

	Για το τερματισμό των workers ο parent στέλνει πρώτα ένα μήνυμα το οποίο τους ενημερώνει να τελειώσουν τη δουλειά και να 		απελευθερώσουν την μνήμη. Οταν οι workers είναι έτοιμη θα στείλουν SIGKILL.

	Ο parent πιάνει τα SIGINT/SIGQUIT φτιάχνει ένα logfile και στέλνει στους workers SIGKILL και τους περιμένει να τελειώσουν.
Query : 
	listCountries -> Διαβάζει τον πίνακα που έχει αποθηκεύσει τα Directories το συνθέτει σε ένα string με το τερματικό σύμβολο και 				το στέλνει στον parent.
	diseaseFrequency -> Αν δοθεί Country ψάχνει στο Hashtable by Country, βρίσκει την χώρα και μετράει τους ασθενείς. Αν το Country 			= NULL ψάχνει στο Hashtable by Disease.
	topkAgeRanges -> Ψάχνει στο Hashtable by Country βρίσκει την χώρα και μετράει τους ασθενείς από το RBT 
	searchPatientRecord -> Διατρέχει την λίστα, αν βρεί το id το γράφει στο pipe, αλλιώς γράφει Error.
	numPatientDischarges -> Αν δοθεί Country ψάχνει στο Hashtable by Country, βρίσκει την χώρα και μετράει τους ασθενείς που έχουν 				exitdate. Αν το Country = NULL διαβάζει όλο το Hashtable by Country και μετράει τους ασθενείς που έχουν 			exitdate για όλες τις χώρες.
	numPatientAdmissions -> Same as above... αλλά μετράει τα ENTRY με την ασθένεια disease.
	exit -> Απελευθερώνει τη μνήμη και στέλνει στον parent ένα μήνυμα ότι τελέιωσε, περιμένει ο parent να του στείλει SIGKILL
