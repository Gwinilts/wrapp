//
// Created by gwinilts on 02/06/20.
//

#include "chess.h"


namespace chs {
	board::board() {
		king = 0;
		side = color::white;
		lastSelect = NULL;
	}

	void board::reset(color side) {
		this->side = side;
		lastSelect = NULL;

		color top;
		color bottom;

		king = shrink(4, 0);

		if (side == color::white) {
			top = color::black;
			bottom = color::white;
		} else {
			top = color::white;
			bottom = color::black;
		}

		for (int i = 0; i < 8; i++) {
			for (int o = 0; o < 8; o++) {
				tile[i][o] = 0;
			}
		}

		for (int i = 0; i < 8; i++) {
			tile[i][6] = ptoc(piece::pawn, top);
			tile[i][1] = ptoc(piece::pawn, bottom);
		}

		tile[0][7] = ptoc(piece::rook, top);
		tile[0][0] = ptoc(piece::rook, bottom);
		tile[7][7] = ptoc(piece::rook, top);
		tile[7][0] = ptoc(piece::rook, bottom);


		tile[1][7] = ptoc(piece::knight, top);
		tile[1][0] = ptoc(piece::knight, bottom);
		tile[6][7] = ptoc(piece::knight, top);
		tile[6][0] = ptoc(piece::knight, bottom);


		tile[2][7] = ptoc(piece::bishop, top);
		tile[2][0] = ptoc(piece::bishop, bottom);
		tile[5][7] = ptoc(piece::bishop, top);
		tile[5][0] = ptoc(piece::bishop, bottom);

		tile[3][7] = ptoc(piece::queen, top);
		tile[4][7] = ptoc(piece::king, top);

		tile[3][0] = ptoc(piece::queen, bottom);
		tile[4][0] = ptoc(piece::king, bottom);
	}

	bool board::isFriendlyOccupied(unsigned char x, unsigned char y) {
		if (tile[x][y] == 0) return false;
		piece_t p = ctop(tile[x][y]);
		return p.side == side;
	}

	bool board::isFriendlyOccupied(unsigned char x, unsigned char y, char _tile[8][8]) {
		if (_tile[x][y] == 0) return false;
		piece_t p = ctop(_tile[x][y]);
		return p.side == side;
	}

	bool board::isOccupied(unsigned char x, unsigned char y) {
		if (x > 7 || y > 7) return false;
		return tile[x][y] != 0;
	}

	bool board::isOccupied(unsigned char x, unsigned char y, char _tile[8][8]) {
		if (x > 7 || y > 7) return false;
		return _tile[x][y] != 0;
	}

	bool board::isPossible(char x, char y) {
		return (x < 8 && y < 8 && x > -1 && y > -1);
	}

	bool board::move(unsigned char pos) {
		unsigned char x, y, _x, _y;
		piece_t p;
		if (lastSelect == NULL) return false;
		for (char i = 0; i < sel_size; i++) {
			if (pos == lastSelect[i]) {
				unshrink(origin, _x, _y);
				p = ctop(tile[_x][_y]);
				unshrink(pos, x, y);
				tile[x][y] = tile[_x][_y];
				tile[_x][_y] = 0;
				delete [] lastSelect;
				lastSelect = NULL;
				sel_size = 0;
				if (p.type == piece::king) king = shrink(_x, _y);
				return true;
			}
		}
		return false;
	}

	void board::move(unsigned char pos, unsigned char dst) {
		unsigned char x, y, _x, _y;

		unshrink(pos, x, y);
		unshrink(dst, _x, _y);

		//piece_t p = ctop(tile[x][y]);

		//x = 7 - x;
		y = 7 - y;
		//_x = 7 - _x;
		_y = 7 - _y;

		tile[_x][_y] = tile[x][y];
		tile[x][y] = 0;

		//if (p.type == piece::king) king = shrink(_x, _y);


	}

	unsigned char board::getOrigin() {
		return origin;
	}

	char board::select(unsigned char pos, unsigned char* &moves) {
		char t = getPossibleMoves(pos, moves);

		if (lastSelect != NULL) delete [] lastSelect;
		lastSelect = NULL;

		if (t < 1) {
			logx("selected nothing");
			return 0;
		}

		sel_size = t;
		origin = pos;
		lastSelect = new unsigned char[t];

		memcpy(lastSelect, moves, t);

		return t;
	}

	char board::getPieces(unsigned char* &pieces) {
		unsigned char _buf[128];
		char fill = 0;

		for (unsigned char i = 0; i < 8; i++) {
			for (unsigned char o = 0; o < 8; o++) {
				if (tile[i][o] != 0) {
					_buf[fill++] = shrink(i, o);
					_buf[fill++] = tile[i][o];
				}
			}
		}

		pieces = new unsigned char[fill];
		memcpy(pieces, _buf, fill);
		return fill;
	}

	board::~board() {
	}

	bool board::willEndCheck(unsigned char pos, unsigned char x, unsigned char y) {
		return !willCauseCheck(pos, x, y);
	}

	bool board::isEnemyOccupied(unsigned char x, unsigned char y) {
		if (tile[x][y] == 0) return false;
		piece_t p = ctop(tile[x][y]);
		return side != p.side;
	}

	bool board::isEnemyOccupied(unsigned char x, unsigned char y, char _tile[8][8]) {
		if (_tile[x][y] == 0) return false;
		piece_t p = ctop(_tile[x][y]);
		return side != p.side;
	}

	char board::getPossibleMoves(unsigned char pos, unsigned char* &moves) {
		unsigned char count = 0, i, x, y;
		unsigned char buf[128];
		char _x, _y;
		piece_t p;

		unshrink(pos, x, y);
		logx("concerning " + to_string(x) + ", " + to_string(y));

		if (x > 7 || y > 7) return 0;

		if (!isFriendlyOccupied(x, y)) return 0;

		if (willCauseCheck(pos, x, y)) {
			logx("note we are in check");
		}

		p = ctop(tile[x][y]);

		switch (p.type) {
		case piece::pawn:
			logx("pawn");
			if (isPossible(x, y + 1) && !isOccupied(x, y + 1)) {
				buf[count++] = shrink(x, y + 1);
				if (y == 1 && !isOccupied(x, y + 2)) {
					buf[count++] = shrink(x, y + 2);
				}
			}
			if (isPossible(x - 1, y + 1) && isEnemyOccupied(x - 1, y + 1)) {
				buf[count++] = shrink(x - 1, y + 1);
			}
			if (isPossible(x + 1, y + 1) && isEnemyOccupied(x + 1, y + 1)) {
				buf[count++] = shrink(x + 1, y + 1);
			}
			break;
		case piece::knight:
			logx("knightly knight");
			for (char mx = -2; mx < 3; mx ++) {
				for (char my = -2; my < 3; my ++) {
					_x = x + mx;
					_y = y + my;
					if (_x == 0 && _y == 2) logx(to_string(mx) + ": " + to_string(my) + "; " + to_string(_x) + ", " + to_string(y));
					if (mx != 0 && my != 0 && (mx != (0 - my)) && (mx != my)) {
						if (isPossible(_x, _y) && !isFriendlyOccupied(_x, _y)) {
							buf[count++] = shrink(_x, _y);
						} else {
							if (isPossible(_x, _y)) {
								logx(to_string(_x) + ", " + to_string(_y));
							}
						}
					}
				}
			}
			break;
		case piece::king:
			logx("kingly king");
			for (char mx = -1; mx < 2; mx++) {
				for (char my = -1; my < 2; my++) {
					if (!(mx == 0 && my == 0)) {
						_x = x + mx;
						_y = y + my;
						if (isPossible(_x, _y)) {
							if (!isFriendlyOccupied(_x, _y)) {
								buf[count++] = shrink(_x, _y);
							}
						}
					}
				}
			}
			break;
		case piece::queen:
			logx("queen");
		case piece::bishop:
			for (char mx = -1; mx < 2; mx += 2) {
				for (char my = -1; my < 2; my += 2) {
					_y = y + my;
					_x = x + mx;

					while (isPossible(_x, _y)) {
						if (isFriendlyOccupied(_x, _y)) break;
						if (isEnemyOccupied(_x, _y)) {
							buf[count++] = shrink(_x, _y);
							break;
						}
						buf[count++] = shrink(_x, _y);
						_x += mx;
						_y += my;
					}
				}
			}
			if (p.type == piece::bishop) {
				logx("bishop");
				break;
			}
		case piece::rook:
			for (char m = -1; m < 2; m += 2) {
				_y = y + m;
				_x = x + m;

				if (m == 0) continue;

				while (isPossible(x, _y)) {
					if (isFriendlyOccupied(x, _y)) break;
					if (isEnemyOccupied(x, _y)) {
						buf[count++] = shrink(x, _y);
						break;
					}
					buf[count++] = shrink(x, _y);
					_y += m;
				}
				while (isPossible(_x, y)) {
					if (isFriendlyOccupied(_x, y)) break;
					if (isEnemyOccupied(_x, y)) {
						buf[count++] = shrink(_x, y);
						break;
					}
					buf[count++] = shrink(_x, y);
					_x += m;
				}
			}
			logx("rook");
			break;
		}

		unshrink(pos, x, y);

		bool isCheck = willCauseCheck(pos, x, y);
		i = 0;

		if (isCheck) {
			logx("am in check atm");
		}

		memcpy(buf + 64, buf, 64);

		for (char t = 0; t < count; t++) {
			unshrink(buf[t + 64], x, y);
			if (!willCauseCheck(pos, x, y)) {
				if (!isCheck) {
					buf[i++] = buf[t + 64];
				} else {
					if (willEndCheck(pos, x, y)) {
						buf[i++] = buf[t + 64];
					}
				}
			} else {
				logx(to_string(x) + ", " + to_string(y) + " would be check");
			}
		}

		moves = new unsigned char[i];
		memcpy(moves, buf, i);
		return i;
	}

	bool board::willCauseCheck(unsigned char pos, unsigned char x, unsigned char y) {
		char _tile[8][8];
		unsigned char _x, _y, kx, ky;

		short restore;
		piece_t old;

		piece_t p;

		for (char i = 0; i < 8; i++) {
			for (char o = 0; o < 8; o++) {
				_tile[i][o] = tile[i][o];
			}
		}

		unshrink(pos, _x, _y);

		unshrink(king, kx, ky);

		p = ctop(_tile[_x][_y]);

		if (p.type == piece::king) {
			kx = x; ky = y;
		}

		_tile[_x][_y] = 0;
		_tile[x][y] = ptoc(p);

		// check for rook or queen

		for (char m = -1; m < 2; m++) {
			_x = kx + m;
			_y = ky + m;

			if (m == 0) continue;

			while (isPossible(kx, _y)) {
				if (isFriendlyOccupied(kx, _y, _tile)) break;
				if (isEnemyOccupied(kx, _y, _tile)) {
					p = ctop(_tile[kx][_y]);
					if ((p.type == piece::queen) || (p.type == piece::rook)) {
						logx("check by rook or queen x axis " + to_string(kx) + ", " + to_string(_y));
						return true;
					}
					break;
				}
				_y += m;
			}

			while (isPossible(_x, ky)) {
				if (isFriendlyOccupied(_x, ky, _tile)) break;
				if (isEnemyOccupied(_x, ky, _tile)) {
					p = ctop(_tile[_x][ky]);
					if ((p.type == piece::queen) || (p.type == piece::rook)) {
						logx("check by rook or queen y axis " + to_string(kx) + ", " + to_string(_x));
						return true;
					}
					break;
				}
				_x += m;
			}
		}

		// queen or bishop

		for (char mx = -1; mx < 2; mx += 2) {
			for (char my = -1; my < 2; my += 2) {
				_y = ky + my;
				_x = kx + mx;

				while (isPossible(_x, _y)) {
					if (isFriendlyOccupied(_x, _y, _tile)) {
						break;
					}
					if (isEnemyOccupied(_x, _y, _tile)) {
						p = ctop(_tile[_x][_y]);
						if (p.type == piece::bishop || p.type == piece::queen) {
							logx("check by queen or bishop " + to_string(x) + ", " + to_string(y));
							return true;
						}
						break;
					}
					_x += mx;
					_y += my;
				}
			}
		}

		// knight

		for (char mx = -2; mx < 3; mx ++) {
			for (char my = -2; my < 3; my ++) {
				_x = kx + mx;
				_y = ky + my;
				if (_y != 0 && _x != 0 && _x != _y && ((((_x + _y) % 2 == -1)) || ((_x + _y) % 2 == 1))) {
					if (isPossible(_x, _y)) {
						if (isEnemyOccupied(_x, _y, _tile)) {
							p = ctop(_tile[_x][_y]);
							if (p.type == piece::knight) {
								logx("check by knight at " + to_string(_x) + ", " + to_string(_y));
								return true;
							}
						}
					}
				}
			}
		}

		//pawn

		if (isPossible(kx + 1, ky + 1)) {
			if (isEnemyOccupied(kx + 1, ky + 1, _tile)) {
				p = ctop(_tile[kx + 1][kx + 1]);
				if (p.type == piece::pawn) {
					return true;
				}
			}
		}

		if (isPossible(kx, ky + 1)) {
			if (isEnemyOccupied(kx, ky + 1, _tile)) {
				p = ctop(_tile[kx][ky + 1]);
				if (p.type == piece::pawn) {
					return true;
				}
			}
		}

		if (isPossible(kx - 1, ky + 1)) {
			if (isEnemyOccupied(kx - 1, ky + 1, _tile)) {
				p = ctop(_tile[kx - 1][ky + 1]);
				if (p.type == piece::pawn) {
					return true;
				}
			}
		}

		return false;
	}
}

chessgame::chessgame(websocket *ws, networklayer *nl) {
	this->ws = ws;
	this->layer = nl;
	hasGame = false;
	hasMove = false;

	_since = new long[(static_cast<unsigned char>(chs::msg_type::MAX) - static_cast<unsigned char>(chs::msg_type::MIN)) + 1];
}

chessgame::~chessgame() {

}

bool chessgame::since(chs::msg_type op, long time) {
	unsigned char index = static_cast<unsigned char>(op) - static_cast<unsigned char>(chs::msg_type::MIN);
	long now;

	std::chrono::milliseconds ms = std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch());

	now = ms.count();

	if (_since[index] == 0 || _since[index] > now) {
		logx("initializing " + to_string(index));
		_since[index] = now;
		return true;
	} else {
		if (now - _since[index] > time) {
			_since[index] = now;
			return true;
		}
	}
	return false;
}

void chessgame::handleSelect(unsigned char x, unsigned char y) {
	unsigned char *moves;
	char *_buf;
	char m;

	m = b.select(chs::shrink(x, y), moves);

	drawBoard();

	if (m > 0) {
		_buf = new char[m];

		memcpy(_buf, moves, m);
		delete moves;

		ws->sendCtl(static_cast<unsigned char>(chs::msg_type::suggest), m, _buf);
	}
}

void chessgame::handleClientMsg(chs::msg_type op, size_t length, char* data) {
	char *_buf;
	char t;

	switch (op) {
	case chs::msg_type::challange:
		_buf = new char[length - 1];
		memcpy(_buf, data + 1, length - 1);
		challanger = string(_buf, length - 1);
		delete _buf;
		logx("challanger is " + challanger);
		break;
	case chs::msg_type::accept:
		if (length == 2) {
			white = (data[1] == 1);
			hasGame = true;
			_buf = new char[1];
			_buf[0] = white ? 0 : 1;
			layer->sendTo(static_cast<unsigned char>(op), challangee, 1, _buf);
			_buf = new char[1];
			_buf[0] = white ? 1 : 0;
			ws->sendCtl(static_cast<unsigned char>(chs::msg_type::init), 1, _buf);
			b.reset(white ? chs::color::white : chs::color::black);
			turn = 1;
			drawBoard();
			opponent = challangee;
		} else {
			logx("bad length");
		}
		break;
	case chs::msg_type::select:
		if ((turn % 2) == (white ? 0 : 1)) {
			return;
		}
		if (b.move(chs::shrink(data[1], data[2]))) {
			// piece has moved, inform opponent
			drawBoard();
			_turn = turn;

			_o = b.getOrigin();
			_m = chs::shrink(data[1], data[2]);
			hasMove = true;


			/*
			_buf = new char[4];
			memcpy(_buf, &turn, 2);
			_buf[2] = b.getOrigin();
			_buf[3] = chs::shrink(data[1], data[2]);

			layer->sendTo(static_cast<unsigned char>(chs::msg_type::select), opponent, 4, _buf);
			*/
			turn++;
			// inform opponent
		} else {
			handleSelect(data[1], data[2]);
		}
		break;
	default:
		logx("couldn't handle client message");
	}
}

void chessgame::drawBoard() {
	unsigned char* pieces;
	char c = b.getPieces(pieces);

	char* _buf = new char[c + 1];

	for (char i = 0; i < c; i++) {
		_buf[i + 1] = pieces[i];
	}

	_buf[0] = white ? 27 : 33;

	delete [] pieces;

	ws->sendCtl(static_cast<unsigned char>(chs::msg_type::draw), c + 1, _buf);
}

void chessgame::handleNetworkMsg(chs::msg_type op, string sender, unsigned short length, char* data) {
	char *_buf;
	short t;

	switch (op) {
	case chs::msg_type::challange:
		if (hasGame) return;
		if (sender == challangee) return;

		_buf = new char[sender.length()];
		memcpy(_buf, sender.c_str(), sender.length());
		ws->sendCtl(static_cast<unsigned char>(op), sender.length(), _buf);
		challangee = sender;
		break;
	case chs::msg_type::accept:
		white = data[0] == 1;
		hasGame = true;
		_buf = new char[1];
		_buf[0] = white ? 1 : 0;
		ws->sendCtl(static_cast<unsigned char>(chs::msg_type::init), 1, _buf);
		b.reset(white ? chs::color::white : chs::color::black);
		opponent = sender;
		turn = 1;
		drawBoard();
		break;
	case chs::msg_type::select:
		if (length == 4) {
			memcpy(&t, data, 2);
			if (t == turn) {
				b.move(data[2], data[3]);
				drawBoard();
				turn++;
			} else {
				//logx("got select for wrong turn (" + to_string(t) + ", " + to_string(turn));
			}
		}
		break;
	default:
		logx("bad type");
	}
}


void chessgame::work() {
	char *_buf;
	if (hasGame) {
		if (hasMove) {
			if (since(chs::msg_type::select, 500)) {
				_buf = new char[4];
				memcpy(_buf, &_turn, 2);
				_buf[2] = _o;
				_buf[3] = _m;
				layer->sendTo(static_cast<unsigned char>(chs::msg_type::select), opponent, 4, _buf);
			}
		}
	} else {
		if (!challanger.empty() && challangee.empty()) {
			if (since(chs::msg_type::challange, 900)) {
				layer->sendTo(static_cast<unsigned char>(chs::msg_type::challange), challanger, 0, NULL);
			}
		}
	}
}

void chessgame::stop() {

}
