#include "RegexGroup.hpp"

#include "RegexFlags.hpp"
#include "RegexTransitionsVector.hpp"
#include "string.hpp"
#include "unicode.hpp"

#include "RegexAnyCharConstraint.hpp"
#include "RegexCharConstraint.hpp"
#include "RegexCodepointConstraint.hpp"
#include "RegexDummyConstraint.hpp"
#include "RegexEndConstraint.hpp"
#include "RegexExactQuantifierConstraint.hpp"
#include "RegexGroupConstraint.hpp"
#include "RegexNegativeLookaheadConstraint.hpp"
#include "RegexNegativeLookbehindConstraint.hpp"
#include "RegexOpenEndedRangeQuantifierConstraint.hpp"
#include "RegexPositiveLookaheadConstraint.hpp"
#include "RegexPositiveLookbehindConstraint.hpp"
#include "RegexOptConstraint.hpp"
#include "RegexRangeQuantifierConstraint.hpp"
#include "RegexRangeConstraint.hpp"
#include "RegexRecallConstraint.hpp"
#include "RegexRepeatConstraint.hpp"
#include "RegexStartConstraint.hpp"
#include "RegexWordBoundaryConstraint.hpp"
#include "RegexWordCharConstraint.hpp"

NAMESPACE_SOUP
{
	static void discharge_alternative(RegexGroup& g, RegexTransitionsVector& success_transitions, RegexAlternative& a)
	{
		// Ensure all alternatives have at least one constraint so we can set up transitions
		if (a.constraints.empty())
		{
			auto upC = soup::make_unique<RegexDummyConstraint>();
			success_transitions.setTransitionTo(upC.get());
			success_transitions.emplace(&upC->success_transition);
			a.constraints.emplace_back(std::move(upC));
		}

		g.alternatives.emplace_back(std::move(a));
		a.constraints.clear();
	}

	RegexGroup::RegexGroup(const ConstructorState& s, bool non_capturing)
		: index(non_capturing ? -1 : s.next_index++)
	{
		RegexTransitionsVector success_transitions;
		success_transitions.data = { &initial };

		RegexAlternative a{};

		std::vector<RegexConstraint**> alternatives_transitions{};

		bool escape = false;
		for (; s.it != s.end; ++s.it)
		{
			if (escape)
			{
				escape = false;
				if (*s.it == 'b')
				{
					auto upC = soup::make_unique<RegexWordBoundaryConstraint<false>>();
					success_transitions.setTransitionTo(upC.get());
					success_transitions.emplace(&upC->success_transition);
					a.constraints.emplace_back(std::move(upC));
					continue;
				}
				else if (*s.it == 'B')
				{
					auto upC = soup::make_unique<RegexWordBoundaryConstraint<true>>();
					success_transitions.setTransitionTo(upC.get());
					success_transitions.emplace(&upC->success_transition);
					a.constraints.emplace_back(std::move(upC));
					continue;
				}
				else if (*s.it == 'w')
				{
					auto upC = soup::make_unique<RegexWordCharConstraint<false>>();
					success_transitions.setTransitionTo(upC.get());
					success_transitions.emplace(&upC->success_transition);
					a.constraints.emplace_back(std::move(upC));
					continue;
				}
				else if (*s.it == 'W')
				{
					auto upC = soup::make_unique<RegexWordCharConstraint<true>>();
					success_transitions.setTransitionTo(upC.get());
					success_transitions.emplace(&upC->success_transition);
					a.constraints.emplace_back(std::move(upC));
					continue;
				}
				else if (*s.it == 'A')
				{
					auto upC = soup::make_unique<RegexStartConstraint<true, false>>();
					success_transitions.setTransitionTo(upC.get());
					success_transitions.emplace(&upC->success_transition);
					a.constraints.emplace_back(std::move(upC));
					continue;
				}
				else if (*s.it == 'Z')
				{
					auto upC = soup::make_unique<RegexEndConstraint<true, false, false>>();
					success_transitions.setTransitionTo(upC.get());
					success_transitions.emplace(&upC->success_transition);
					a.constraints.emplace_back(std::move(upC));
					continue;
				}
				else if (*s.it == 'z')
				{
					auto upC = soup::make_unique<RegexEndConstraint<true, false, true>>();
					success_transitions.setTransitionTo(upC.get());
					success_transitions.emplace(&upC->success_transition);
					a.constraints.emplace_back(std::move(upC));
					continue;
				}
				else if (*s.it == 'd')
				{
					auto upC = soup::make_unique<RegexRangeConstraint>(RegexRangeConstraint::digits);
					success_transitions.setTransitionTo(upC.get());
					success_transitions.emplace(&upC->success_transition);
					a.constraints.emplace_back(std::move(upC));
					continue;
				}
				else if (*s.it == 's')
				{
					auto upC = soup::make_unique<RegexRangeConstraint>(RegexRangeConstraint::whitespace);
					success_transitions.setTransitionTo(upC.get());
					success_transitions.emplace(&upC->success_transition);
					a.constraints.emplace_back(std::move(upC));
					continue;
				}
				else if (*s.it == 'k')
				{
					if (++s.it != s.end)
					{
						std::string name;
						if (*s.it == '<')
						{
							while (++s.it != s.end && *s.it != '>')
							{
								name.push_back(*s.it);
							}
						}
						else
						{
							while (++s.it != s.end && *s.it != '\'')
							{
								name.push_back(*s.it);
							}
						}

						auto upC = soup::make_unique<RegexRecallNameConstraint>(std::move(name));
						success_transitions.setTransitionTo(upC.get());
						success_transitions.emplace(&upC->success_transition);
						a.constraints.emplace_back(std::move(upC));
					}
					continue;
				}
			}
			else
			{
				if (*s.it == '\\')
				{
					if (++s.it != s.end && string::isNumberChar(*s.it))
					{
						size_t i = ((*s.it) - '0');
						while (++s.it != s.end && string::isNumberChar(*s.it))
						{
							i *= 10;
							i += ((*s.it) - '0');
						}

						auto upC = soup::make_unique<RegexRecallIndexConstraint>(i);
						success_transitions.setTransitionTo(upC.get());
						success_transitions.emplace(&upC->success_transition);
						a.constraints.emplace_back(std::move(upC));
					}
					else
					{
						escape = true;
					}
					--s.it;
					continue;
				}
				else if (*s.it == '|')
				{
					discharge_alternative(*this, success_transitions, a);
					success_transitions.discharge(alternatives_transitions);
					continue;
				}
				else if (*s.it == '(')
				{
					bool non_capturing = false;
					bool positive_lookahead = false;
					bool negative_lookahead = false;
					bool positive_lookbehind = false;
					bool negative_lookbehind = false;
					std::string name{};
					std::string inline_modifiers{};
					if (++s.it != s.end && *s.it == '?')
					{
						while (++s.it != s.end && (*s.it == '-' || string::isLetter(*s.it)))
						{
							inline_modifiers.push_back(*s.it);
						}
						if (s.it != s.end)
						{
							if (*s.it == ':')
							{
								++s.it;
								non_capturing = true;
							}
							else if (*s.it == '\'')
							{
								while (++s.it != s.end && *s.it != '\'')
								{
									name.push_back(*s.it);
								}
								if (s.it != s.end)
								{
									++s.it;
								}
							}
							else if (*s.it == '=')
							{
								positive_lookahead = true;
								++s.it;
							}
							else if (*s.it == '!')
							{
								negative_lookahead = true;
								++s.it;
							}
							else if (*s.it == '<')
							{
								if (++s.it != s.end)
								{
									if (*s.it == '=')
									{
										positive_lookbehind = true;
										++s.it;
									}
									else if (*s.it == '!')
									{
										negative_lookbehind = true;
										++s.it;
									}
									else
									{
										do
										{
											name.push_back(*s.it);
										} while (++s.it != s.end && *s.it != '>');
										if (s.it != s.end)
										{
											++s.it;
										}
									}
								}
							}
						}
					}
					uint16_t restore_flags = s.flags;
					if (!inline_modifiers.empty())
					{
						std::string negative_inline_modifiers{};
						auto sep = inline_modifiers.find('-');
						if (sep != std::string::npos)
						{
							negative_inline_modifiers = inline_modifiers.substr(sep + 1);
							inline_modifiers.erase(0, sep + 1);
						}
						s.flags |= Regex::parseFlags(inline_modifiers.c_str());
						s.flags &= ~Regex::parseFlags(negative_inline_modifiers.c_str());

						// If non_capturing is true, these are supposed to be localised inline modifers.
						if (!non_capturing)
						{
							// Otherwise, we want to keep them beyond the scope of this group.
							restore_flags = s.flags;

							// However, this kind of group should always be non-capturing.
							non_capturing = true;
						}
					}
					if (positive_lookahead)
					{
						auto upGC = soup::make_unique<RegexPositiveLookaheadConstraint>(s);
						upGC->group.parent = this;
						upGC->group.lookahead_or_lookbehind = true;

						if (upGC->group.initial)
						{
							// last-constraint --[success]-> first-lookahead-constraint + save checkpoint
							success_transitions.setTransitionTo(upGC->group.initial, true);
							success_transitions.data = std::move(s.alternatives_transitions);

							// last-lookahead-constraint --[success]-> group (to restore checkpoint)
							success_transitions.setTransitionTo(upGC.get());

							// group --> next-constraint
							success_transitions.emplace(&upGC->success_transition);
						}

						a.constraints.emplace_back(std::move(upGC));
					}
					else if (negative_lookahead)
					{
						auto upGC = soup::make_unique<RegexNegativeLookaheadConstraint>(s);
						upGC->group.parent = this;
						upGC->group.lookahead_or_lookbehind = true;

						if (upGC->group.initial)
						{
							// last-constraint --[success]-> first-lookahead-constraint
							success_transitions.setTransitionTo(upGC->group.initial);
							success_transitions.data = std::move(s.alternatives_transitions);
						}

						// last-lookahead-constraint --[success]-> fail
						success_transitions.setTransitionTo(RegexConstraint::SUCCESS_TO_FAIL);

						if (upGC->group.initial)
						{
							// first-lookahead-constraint --[rollback]-> next-constraint
							success_transitions.emplaceRollback(&upGC->group.initial->rollback_transition);
						}

						a.constraints.emplace_back(std::move(upGC));
					}
					else if (positive_lookbehind)
					{
						UniquePtr<RegexConstraintLookbehind> upGC;
						if (s.hasFlag(RE_UNICODE))
						{
							upGC = soup::make_unique<RegexPositiveLookbehindConstraint<true>>(s);
						}
						else
						{
							upGC = soup::make_unique<RegexPositiveLookbehindConstraint<false>>(s);
						}
						upGC->group.parent = this;
						upGC->group.lookahead_or_lookbehind = true;
						upGC->window = upGC->group.getCursorAdvancement();

						// last-constraint --[success]-> group (to move cursor)
						success_transitions.setTransitionTo(upGC.get());

						// group --> first-lookbehind-constraint
						success_transitions.emplace(&upGC->success_transition);
						success_transitions.setTransitionTo(upGC->group.initial);

						// last-lookbehind-constraint --[success]-> next-constraint
						success_transitions.data = std::move(s.alternatives_transitions);

						a.constraints.emplace_back(std::move(upGC));
					}
					else if (negative_lookbehind)
					{
						UniquePtr<RegexConstraintLookbehind> upGC;
						if (s.hasFlag(RE_UNICODE))
						{
							upGC = soup::make_unique<RegexNegativeLookbehindConstraint<true>>(s);
						}
						else
						{
							upGC = soup::make_unique<RegexNegativeLookbehindConstraint<false>>(s);
						}
						upGC->group.parent = this;
						upGC->group.lookahead_or_lookbehind = true;
						upGC->window = upGC->group.getCursorAdvancement();

						// last-constraint --[success]-> group (to move cursor)
						success_transitions.setTransitionTo(upGC.get());

						// group --> first-lookbehind-constraint
						success_transitions.emplace(&upGC->success_transition);
						success_transitions.setTransitionTo(upGC->group.initial);

						// last-lookbehind-constraint --[success]-> fail
						success_transitions.data = std::move(s.alternatives_transitions);
						success_transitions.setTransitionTo(RegexConstraint::SUCCESS_TO_FAIL);

						// group --[rollback]--> next-constraint
						success_transitions.emplaceRollback(&upGC->rollback_transition);

						a.constraints.emplace_back(std::move(upGC));
					}
					else
					{
						if (s.hasFlag(RE_EXPLICIT_CAPTURE) && name.empty())
						{
							non_capturing = true;
						}
						if (*s.it == ')' // No contents?
							&& non_capturing // Not a capturing group?
							)
						{
							// Don't have to generate anything for this group.
						}
						else
						{
							auto upGC = soup::make_unique<RegexGroupConstraint>(s, non_capturing);
							upGC->data.parent = this;
							upGC->data.name = std::move(name);
							success_transitions.setTransitionTo(upGC.get());
							success_transitions.emplace(&upGC->success_transition);
							success_transitions.setTransitionTo(upGC->data.initial);
							success_transitions.data = std::move(s.alternatives_transitions);
							a.constraints.emplace_back(std::move(upGC));
						}
						s.flags = restore_flags;
					}
					if (s.it == s.end)
					{
						break;
					}
					continue;
				}
				else if (*s.it == ')')
				{
					break;
				}
				else if (*s.it == '+')
				{
					bool greedy = true;
					if (s.it + 1 != s.end
						&& *(s.it + 1) == '?'
						)
					{
						greedy = false;
						++s.it;
					}
					greedy ^= s.hasFlag(RE_UNGREEDY);

					SOUP_ASSERT(!a.constraints.empty(), "Invalid modifier");
					RegexConstraint* pModifiedConstraint;
					UniquePtr<RegexConstraint> upQuantifierConstraint;
					if (greedy)
					{
						UniquePtr<RegexConstraint> upModifiedConstraint = std::move(a.constraints.back());
						pModifiedConstraint = upModifiedConstraint.get();
						upQuantifierConstraint = soup::make_unique<RegexRepeatConstraint<true, true>>(std::move(upModifiedConstraint));
						static_cast<RegexRepeatConstraint<true, true>*>(upQuantifierConstraint.get())->setupTransitionsAtLeastOne(success_transitions);
					}
					else
					{
						UniquePtr<RegexConstraint> upModifiedConstraint = std::move(a.constraints.back());
						pModifiedConstraint = upModifiedConstraint.get();
						upQuantifierConstraint = soup::make_unique<RegexRepeatConstraint<true, false>>(std::move(upModifiedConstraint));
						static_cast<RegexRepeatConstraint<true, false>*>(upQuantifierConstraint.get())->setupTransitionsAtLeastOne(success_transitions);
					}

					pModifiedConstraint->group = this;

					a.constraints.back() = std::move(upQuantifierConstraint);
					continue;
				}
				else if (*s.it == '*')
				{
					bool greedy = true;
					if (s.it + 1 != s.end
						&& *(s.it + 1) == '?'
						)
					{
						greedy = false;
						++s.it;
					}
					greedy ^= s.hasFlag(RE_UNGREEDY);

					SOUP_ASSERT(!a.constraints.empty(), "Invalid modifier");
					RegexConstraint* pModifiedConstraint;
					UniquePtr<RegexConstraint> upQuantifierConstraint;
					if (greedy)
					{
						UniquePtr<RegexConstraint> upModifiedConstraint = std::move(a.constraints.back());
						pModifiedConstraint = upModifiedConstraint.get();
						upQuantifierConstraint = soup::make_unique<RegexRepeatConstraint<false, true>>(std::move(upModifiedConstraint));
					}
					else
					{
						UniquePtr<RegexConstraint> upModifiedConstraint = std::move(a.constraints.back());
						pModifiedConstraint = upModifiedConstraint.get();
						upQuantifierConstraint = soup::make_unique<RegexRepeatConstraint<false, false>>(std::move(upModifiedConstraint));
					}

					pModifiedConstraint->group = this;

					if (greedy)
					{
						// constraint --[success]-> constraint
						success_transitions.setTransitionTo(pModifiedConstraint->getEntrypoint());

						// constraint --[rollback]-> next-constraint
						success_transitions.emplaceRollback(&pModifiedConstraint->rollback_transition);
					}
					else
					{
						// prev-constraint --[success]-> quantifier
						success_transitions.setPreviousTransitionTo(upQuantifierConstraint.get());

						// constraint --[success]-> quantifier
						success_transitions.setTransitionTo(upQuantifierConstraint.get());

						// quantifier --[success]-> next-constraint
						success_transitions.emplace(&upQuantifierConstraint->success_transition);

						// quantifier --[rollback]-> constraint
						upQuantifierConstraint->rollback_transition = pModifiedConstraint->getEntrypoint();
					}

					a.constraints.back() = std::move(upQuantifierConstraint);
					continue;
				}
				else if (*s.it == '?')
				{
					SOUP_ASSERT(!a.constraints.empty(), "Invalid modifier");
					UniquePtr<RegexConstraint> upModifiedConstraint = std::move(a.constraints.back());
					auto pModifiedConstraint = upModifiedConstraint.get();
					auto upOptConstraint = soup::make_unique<RegexOptConstraint>(std::move(upModifiedConstraint));

					pModifiedConstraint->group = this;

					// constraint --[rollback]-> next-constraint
					success_transitions.emplaceRollback(&pModifiedConstraint->getEntrypoint()->rollback_transition);

					a.constraints.back() = std::move(upOptConstraint);
					continue;
				}
				else if (*s.it == '.')
				{
					UniquePtr<RegexConstraint> upC;
					if (s.hasFlag(RE_DOTALL))
					{
						if (s.hasFlag(RE_UNICODE))
						{
							upC = soup::make_unique<RegexAnyCharConstraint<true, true>>();
						}
						else
						{
							upC = soup::make_unique<RegexAnyCharConstraint<true, false>>();
						}
					}
					else
					{
						if (s.hasFlag(RE_UNICODE))
						{
							upC = soup::make_unique<RegexAnyCharConstraint<false, true>>();
						}
						else
						{
							upC = soup::make_unique<RegexAnyCharConstraint<false, false>>();
						}
					}
					success_transitions.setTransitionTo(upC.get());
					success_transitions.emplace(&upC->success_transition);
					a.constraints.emplace_back(std::move(upC));
					continue;
				}
				else if (*s.it == '[')
				{
					auto upC = soup::make_unique<RegexRangeConstraint>(s.it, s.end, s.hasFlag(RE_INSENSITIVE));
					success_transitions.setTransitionTo(upC.get());
					success_transitions.emplace(&upC->success_transition);
					a.constraints.emplace_back(std::move(upC));
					if (s.it == s.end)
					{
						break;
					}
					continue;
				}
				else if (*s.it == '^')
				{
					UniquePtr<RegexConstraint> upC;
					if (s.flags & RE_MULTILINE)
					{
						upC = soup::make_unique<RegexStartConstraint<false, true>>();
					}
					else
					{
						upC = soup::make_unique<RegexStartConstraint<false, false>>();
					}
					success_transitions.setTransitionTo(upC.get());
					success_transitions.emplace(&upC->success_transition);
					a.constraints.emplace_back(std::move(upC));
					continue;
				}
				else if (*s.it == '$')
				{
					UniquePtr<RegexConstraint> upC;
					if (s.flags & RE_MULTILINE)
					{
						upC = soup::make_unique<RegexEndConstraint<false, true, false>>();
					}
					else if (s.flags & RE_DOLLAR_ENDONLY)
					{
						upC = soup::make_unique<RegexEndConstraint<false, false, true>>();
					}
					else
					{
						upC = soup::make_unique<RegexEndConstraint<false, false, false>>();
					}
					success_transitions.setTransitionTo(upC.get());
					success_transitions.emplace(&upC->success_transition);
					a.constraints.emplace_back(std::move(upC));
					continue;
				}
				else if (*s.it == '{')
				{
					size_t min_reps = 0;
					while (++s.it != s.end && string::isNumberChar(*s.it))
					{
						min_reps *= 10;
						min_reps += ((*s.it) - '0');
					}
					if (s.it == s.end)
					{
						break;
					}

					bool exact = true;
					size_t max_reps = 0;
					if (*s.it == ',')
					{
						exact = false;
						while (++s.it != s.end && string::isNumberChar(*s.it))
						{
							max_reps *= 10;
							max_reps += ((*s.it) - '0');
						}
						if (s.it == s.end)
						{
							break;
						}
					}

					bool greedy = true;
					if (s.it + 1 != s.end
						&& *(s.it + 1) == '?'
						)
					{
						greedy = false;
						++s.it;
					}
					greedy ^= s.hasFlag(RE_UNGREEDY);

					SOUP_ASSERT(!a.constraints.empty(), "Invalid modifier");
					UniquePtr<RegexConstraint> upModifiedConstraint = std::move(a.constraints.back());
					auto pModifiedConstraint = upModifiedConstraint.get();
					if (min_reps == 0)
					{
						success_transitions.rollback();
						a.constraints.pop_back();
					}
					else if (exact || min_reps == max_reps) // {X} or {X,X}
					{
						// greedy or not doesn't make a difference here

						auto upRepConstraint = soup::make_unique<RegexExactQuantifierConstraint>();
						upRepConstraint->constraints.emplace_back(std::move(upModifiedConstraint));

						pModifiedConstraint->group = this;

						while (--min_reps != 0)
						{
							if (pModifiedConstraint->shouldResetCapture())
							{
								success_transitions.setResetCapture();
							}
							auto upClone = pModifiedConstraint->clone(success_transitions);
							upClone->group = this;

							upRepConstraint->constraints.emplace_back(std::move(upClone));
						}
						a.constraints.back() = std::move(upRepConstraint);
					}
					else if (max_reps == 0) // {X,}
					{
						UniquePtr<RegexOpenEndedRangeQuantifierConstraintBase> upRepConstraint;
						if (greedy)
						{
							upRepConstraint = soup::make_unique<RegexOpenEndedRangeQuantifierConstraint<true>>();
						}
						else
						{
							upRepConstraint = soup::make_unique<RegexOpenEndedRangeQuantifierConstraint<false>>();
						}
						upRepConstraint->constraints.emplace_back(std::move(upModifiedConstraint));

						pModifiedConstraint->group = this;

						while (--min_reps != 0)
						{
							if (pModifiedConstraint->shouldResetCapture())
							{
								success_transitions.setResetCapture();
							}
							auto upClone = pModifiedConstraint->clone(success_transitions);
							upClone->group = this;

							upRepConstraint->constraints.emplace_back(std::move(upClone));
						}

						// last-clone --[success]-> quantifier
						success_transitions.setTransitionTo(upRepConstraint.get());

						if (greedy)
						{
							// quantifier --[success]-> last-clone
							success_transitions.emplace(&upRepConstraint->success_transition);
							if (pModifiedConstraint->shouldResetCapture())
							{
								success_transitions.setResetCapture();
							}
							success_transitions.setTransitionTo(upRepConstraint->constraints.back()->getEntrypoint());

							// quantifier --[rollback]-> next-constraint
							success_transitions.emplaceRollback(&upRepConstraint->rollback_transition);
						}
						else
						{
							// quantifier --[rollback]-> last-clone
							success_transitions.emplaceRollback(&upRepConstraint->rollback_transition);
							if (pModifiedConstraint->shouldResetCapture())
							{
								success_transitions.setResetCapture();
							}
							success_transitions.setTransitionTo(upRepConstraint->constraints.back()->getEntrypoint());

							// quantifier --[success]-> next-constraint
							success_transitions.emplace(&upRepConstraint->success_transition);
						}

						a.constraints.back() = std::move(upRepConstraint);
					}
					else if (min_reps < max_reps) // {X,Y}
					{
						if (greedy)
						{
							auto upRepConstraint = soup::make_unique<RegexRangeQuantifierConstraintGreedy>();
							upRepConstraint->constraints.emplace_back(std::move(upModifiedConstraint));
							upRepConstraint->min_reps = min_reps;

							pModifiedConstraint->group = this;

							size_t required_reps = min_reps;
							while (--required_reps != 0)
							{
								if (pModifiedConstraint->shouldResetCapture())
								{
									success_transitions.setResetCapture();
								}
								auto upClone = pModifiedConstraint->clone(success_transitions);
								upClone->group = this;

								upRepConstraint->constraints.emplace_back(std::move(upClone));
							}
							RegexTransitionsVector rep_transitions;
							success_transitions.discharge(rep_transitions.data);
							for (size_t optional_reps = (max_reps - min_reps); optional_reps != 0; --optional_reps)
							{
								if (pModifiedConstraint->shouldResetCapture())
								{
									rep_transitions.setResetCapture();
								}
								auto upClone = pModifiedConstraint->clone(rep_transitions);
								upClone->group = this;

								// clone --[rollback]-> next-constraint
								success_transitions.emplaceRollback(&upClone->getEntrypoint()->rollback_transition);

								upRepConstraint->constraints.emplace_back(std::move(upClone));
							}

							// last-clone --[success]-> next-constraint
							rep_transitions.discharge(success_transitions.data);

							a.constraints.back() = std::move(upRepConstraint);
						}
						else
						{
							auto upRepConstraint = soup::make_unique<RegexRangeQuantifierConstraintLazy>();
							upRepConstraint->constraints.emplace_back(std::move(upModifiedConstraint));
							upRepConstraint->min_reps = min_reps;

							pModifiedConstraint->group = this;

							size_t required_reps = min_reps;
							while (--required_reps != 0)
							{
								if (pModifiedConstraint->shouldResetCapture())
								{
									success_transitions.setResetCapture();
								}
								auto upClone = pModifiedConstraint->clone(success_transitions);
								upClone->group = this;

								upRepConstraint->constraints.emplace_back(std::move(upClone));
							}

							RegexTransitionsVector rep_transitions;
							success_transitions.discharge(rep_transitions.data);
							for (size_t optional_reps = (max_reps - min_reps); optional_reps != 0; --optional_reps)
							{
								auto upDummy = soup::make_unique<RegexDummyConstraint>();

								// last-constraint --[success]-> dummy
								rep_transitions.setTransitionTo(upDummy->getEntrypoint());

								// dummy --[success]-> next-constraint
								success_transitions.emplace(&upDummy->success_transition);

								// clone --[success]-> next-dummy
								auto upClone = pModifiedConstraint->clone(rep_transitions);
								upClone->group = this;

								// dummy --[rollback]-> clone
								upDummy->rollback_transition = upClone->getEntrypoint();

								upRepConstraint->constraints.emplace_back(std::move(upClone));
								upRepConstraint->constraints.emplace_back(std::move(upDummy));
							}

							// last-clone --[success]-> next-constraint
							rep_transitions.discharge(success_transitions.data);

							a.constraints.back() = std::move(upRepConstraint);
						}
					}
					else
					{
						// We may be here if (!exact && min_reps > max_reps)
						// Which is invalid, so we just yeet the constraint as if {0} was written.
						success_transitions.rollback();
						a.constraints.pop_back();
					}
					continue;
				}

				if (s.hasFlag(RE_EXTENDED))
				{
					if (string::isSpace(*s.it))
					{
						continue;
					}
					if (*s.it == '#')
					{
						do
						{
							++s.it;
						} while (s.it != s.end && *s.it != '\n');
						continue;
					}
				}
			}

			UniquePtr<RegexConstraint> upC;
			if (UTF8_HAS_CONTINUATION(*s.it) && s.hasFlag(RE_UNICODE))
			{
				std::string c;
				do
				{
					c.push_back(*s.it);
				} while (s.it + 1 != s.end && UTF8_IS_CONTINUATION(*++s.it));
				upC = soup::make_unique<RegexCodepointConstraint>(std::move(c));
			}
			else if (s.hasFlag(RE_INSENSITIVE) && string::lower_char(*s.it) != string::upper_char(*s.it))
			{
				const char arr[] = { string::lower_char(*s.it), string::upper_char(*s.it) };
				upC = soup::make_unique<RegexRangeConstraint>(arr);
			}
			else
			{
				upC = soup::make_unique<RegexCharConstraint>(*s.it);
			}
			success_transitions.setTransitionTo(upC.get());
			success_transitions.emplace(&upC->success_transition);
			a.constraints.emplace_back(std::move(upC));
		}
		discharge_alternative(*this, success_transitions, a);
		success_transitions.discharge(alternatives_transitions);

		if (alternatives.size() > 1)
		{
			// Set up rollback transitions for the first constraint in each alternative to jump to next alternative
			for (size_t i = 0; i + 1 != alternatives.size(); ++i)
			{
				if (alternatives.at(i).constraints.at(0)->rollback_transition)
				{
					auto dummy = soup::make_unique<RegexDummyConstraint>();
					dummy->success_transition = alternatives.at(i).constraints.at(0).get();
					dummy->rollback_transition = alternatives.at(i + 1).constraints.at(0)->getEntrypoint();
					alternatives.at(i).constraints.emplace(alternatives.at(i).constraints.begin(), std::move(dummy));
				}
				else
				{
					alternatives.at(i).constraints.at(0)->rollback_transition = alternatives.at(i + 1).constraints.at(0)->getEntrypoint();
				}
			}
		}

		// Set up group pointers 
		for (const auto& a : alternatives)
		{
			for (const auto& c : a.constraints)
			{
				c->group = this;
			}
		}

		s.alternatives_transitions = std::move(alternatives_transitions);
	}

	std::string RegexGroup::toString() const SOUP_EXCAL
	{
		std::string str{};
		for (const auto& a : alternatives)
		{
			for (const auto& c : a.constraints)
			{
				str.append(c->toString());
			}
			str.push_back('|');
		}
		if (!str.empty())
		{
			str.pop_back();
		}
		return str;
	}

	uint16_t RegexGroup::getFlags() const
	{
		uint16_t set = 0;
		uint16_t unset = 0;
		getFlags(set, unset);
		SOUP_ASSERT((set & unset) == 0, "RegexGroup has contradicting flags");
		return set;
	}

	void RegexGroup::getFlags(uint16_t& set, uint16_t& unset) const noexcept
	{
		for (const auto& a : alternatives)
		{
			for (const auto& c : a.constraints)
			{
				c->getFlags(set, unset);
			}
		}
	}

	size_t RegexGroup::getCursorAdvancement() const
	{
		size_t accum = 0;
		for (const auto& a : alternatives)
		{
			for (const auto& c : a.constraints)
			{
				accum += c->getCursorAdvancement();
			}
		}
		return accum;
	}
}
