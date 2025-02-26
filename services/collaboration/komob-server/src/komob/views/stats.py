import datetime
from flask import Module, make_response, abort, current_app, jsonify
from komob import models
from komob.db import redisco

module = Module(__name__)

db = redisco.get_client()

_model_names = {
    models.Session: 'sessions',
    models.TextObj: 'texts',
    models.ViewObj: 'views',
    models.User: 'users',
    models.UserRelation: 'friendships',
    models.Privilege: 'privileges'
}

@module.route('/today/')
def today_stats():
    return daily_stats(datetime.date.today())

@module.route('/latest/')
def latest_stats():
    # TODO store daily total to redis / load *daily* total or return null
    yesterday = datetime.date.today() - datetime.timedelta(days=1)
    return daily_stats(yesterday, latest_totals=True)

@module.route('/<int:date>/')
def daily_stats(date, latest_totals=False):
    """Returns the stats for `date`. If `latest_totals` is True, totals are
    calculated and written to redis with the given date. Otherwise they are
    just looked up in redis. If a total cannot be found for `date` it will be
    null in the JSON output."""
    if not isinstance(date, datetime.date):
        date = _parse_date_or_404(date)
    stats = {}
    stats['total'] = {}
    stats['daily'] = {}
    for model, label in _model_names.iteritems():
        stats['total'][label] = _model_total(model, date, latest_totals)
        stats['daily'][label] = _model_daily(model, date)
    stats['total']['text_length'] = _total_text_length(date, latest_totals)
    date_str = datetime.date.strftime(date, '%Y%m%d')
    return jsonify({ date_str: stats })

def _model_total(model_class, date, persist=False):
    """If persist is true, calculates the current total and stores it under the
    given date in redis for later reference. Returns the total stored for the
    given date otherwise. Returns `None` if no such total exists in redis."""
    if not (isinstance(model_class, models.BaseModel.__class__) and
            isinstance(date, datetime.date)):
        raise TypeError()
    key = _model_total_counter_key(model_class, date)
    if persist:
        count = len(model_class.objects.all())
        db.set(key, count)
    else:
        count = db.get(key)
    return count

def _model_daily(model_class, date):
    """Returns the number of created instances of `model_class` on `date`."""
    if not (isinstance(model_class, models.BaseModel.__class__)
            and isinstance(date, datetime.date)):
        raise TypeError()
    key = model_class._model_counter_key(date)
    return int(db.get(key) or 0)

def _total_text_length(date, persist=False):
    """If `persist` is true, calculates the total text length and stores in redis
    under the given date. Returns the stored length for that date or `None` if
    there is no such value. Note that copies of the texts are also stored in
    the associated `ViewObj` instances, which are not added to this
    calculation."""
    key = '_komob_text_length_%s_total'
    key %= datetime.date.strftime(date, '%Y%m%d')
    length = None
    if persist:
        length = _calc_total_text_length()
        db.set(key, length)
    else:
        length = db.get(key)
    return length

def _calc_total_text_length():
    return sum([len(t._text) for t in models.TextObj.objects.all() if t._text])

def _parse_date_or_404(date):
    try:
        return datetime.datetime.strptime(str(date), '%Y%m%d').date()
    except ValueError:
        current_app.logger.debug('Invalid date format %s' % date)
        abort(404)

def _model_total_counter_key(model_class, date):
    if not isinstance(model_class, models.BaseModel.__class__):
        raise TypeError()
    return model_class._model_counter_key(date) + '_total'