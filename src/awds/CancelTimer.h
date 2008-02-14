#ifndef _CANCELTIMER_H__
#define _CANCELTIMER_H__

#include <gea/API.h>
#include <gea/Blocker.h>

namespace awds {

class CancelTimer {

    friend class CancelTimerManager;
protected:
    CancelTimer *next;

    gea::Blocker blocker;

    typedef void (*callback_t)( CancelTimer *timer, void *data);

    callback_t callback;
    void *data;

    enum State {
	Active,
	Canceled,
	Fired
    };

    enum State state;

    void startTimer(gea::AbsTime timeout, callback_t callback, void *data) {
	this->callback = callback;
	this->data = data;
	this->state = Active;
	GEA.waitFor(&blocker, timeout, timeout_func, this);
    }


    static void timeout_func(gea::Handle *h, gea::AbsTime t, void *self_p) {

	CancelTimer *self = static_cast<CancelTimer *>(self_p);

	//	GEA.dbg() << "timeout happend for " << self << std::endl;

	if (self->state == Active) {
	    self->callback( self, self->data);
	    self->state = Fired;
	    //GEA.dbg() << "calling timout callback" << std::endl;
	}
	// wo wird das Object gelöscht?
    }

public:
    void cancel() {

	//	GEA.dbg() << "timer " << this << " canceled" << std::endl;

	this->state = Canceled;
    }
};

class CancelTimerManager {
    CancelTimer *head;

public:
    CancelTimerManager() {
	head = 0;
    }

    CancelTimer *startTimer(gea::AbsTime timeout, CancelTimer::callback_t callback, void *data) {
	CancelTimer *ct = new CancelTimer;
	ct->next = this->head;
	this->head = ct;
	ct->startTimer(timeout, callback, data);
	cleanUp();
	return ct;
    }

    void cleanUp() {

	CancelTimer *current;
	CancelTimer **prev = &head;

	while( (current = *prev) != 0 ) {
	    if (current->state != CancelTimer::Active) {
		*prev = current->next; // unlink
		delete current;       // and remove
	    }
	    *prev = (*prev)->next;
	}
    }

};

}

#endif //CANCELTIMER_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
