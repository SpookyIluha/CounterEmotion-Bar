#ifndef EMOTIONS_H
#define EMOTIONS_H

#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>
#include <t3d/t3danim.h>
#include <t3d/t3ddebug.h>

const char* backstory_list_happy[10][3] = {
    // 0-10% радости
    {
        "Bartender, I was recently hit by a car. I feel terrible, nothing brings me joy.",
        "A lot of time has passed since my friend's death, but I'm still depressed. Joy has almost disappeared.",
        "Life feels hopeless. Everything is bad."
    },
    // 10-20% радости
    {
        "I recently took a loan for a new car. Not very happy yet, more worried about the debt.",
        "Got a fine for a violation, mood is below zero. Can't find joy.",
        "Paid off part of the debt — feeling a bit better, but still hard."
    },
    // 20-30% радости
    {
        "Paid off part of the loan, feeling relieved. Joy is still weak.",
        "Debt problems continue, mood is not very good.",
        "Promised a promotion at work — lifted my mood a bit."
    },
    // 30-40% радости
    {
        "Paid off the loan completely! It's a bit easier to breathe and smile.",
        "Got out of the debt hole — feeling better, though not at the peak of happiness.",
        "Received good news — spirit lifted a bit."
    },
    // 40-50% радости
    {
        "Won a contest — feeling great! Joy is overflowing.",
        "Found a new job — mood has improved significantly.",
        "Meeting with friends lifted my spirits — feeling good."
    },
    // 50-60% радости
    {
        "Got a car as a reward — I'm very pleased!",
        "Got a promotion at work — mood is at its peak.",
        "Retired — feeling happy and free."
    },
    // 60-70% радости
    {
        "Moved to a new house — feeling true happiness!",
        "Meeting with my loved one made my day special.",
        "Got a pet — now every day brings me joy."
    },
    // 70-80% радости
    {
        "Dream came true: I started my own business! Feeling incredible joy.",
        "Received an award for work — mood is sky-high!",
        "Celebrated an important event with friends — heart full of happiness."
    },
    // 80-90% радости
    {
        "My family is healthy and happy — that's the best reward!",
        "I achieved all my goals for the year — feeling fantastic.",
        "A trip abroad gave unforgettable impressions and joy."
    },
    // 90-100% радости
    {
        "I won a big prize! Can't believe my luck!",
        "Dream came true: I have everything I ever wished for. Feeling absolute happiness.",
        "All my efforts paid off: life is wonderful!"
    }
};

const char* backstory_list_sad[10][3] = {
    // 0% грусти
    {
        "Bartender, I just came to have a drink, nothing special. Life goes on, and I don't feel sad at all.",
        "You know, sometimes you just want to sit in silence. Everything is fine, just in that mood.",
        "No special events — sometimes life flows calmly and without strong emotions at all."
    },
    // 10% грусти
    {
        "I recently lost my job. It's not that bad, but sometimes it feels like everything is going wrong.",
        "I'm a bit tired of the routine, but overall everything is okay. Just sometimes want a change.",
        "My wife went on a business trip for a month. I feel a bit lonely, but it's not that bad."
    },
    // 20% грусти
    {
        "I took a loan for home repairs. I feel a bit vulnerable when I think about debts and future payments.",
        "My grandmother got sick. Time passes quickly, and I realize more care is needed.",
        "I recently broke up with my girlfriend. Inside, I feel a bit empty and sad from the loss."
    },
    // 30% грусти
    {
        "My friend's car was recently hit by a car. He was very upset, and I felt guilty and a bit empty.",
        "I lost my favorite job due to layoffs. I feel a bit lost and hopeless.",
        "My cat died last week. It was very hard to see him weak and tired — my heart ached with pain."
    },
    // 40% грусти
    {
        "I paid off all the debts for the apartment. I feel relief, but inside remains a shadow of sadness for lost time and experiences.",
        "My sister went abroad for a year. I miss her a lot — the house became quiet and empty.",
        "I found out about a close person's illness. Inside, it feels like I'm torn by pain and helplessness — very hard to see their suffering."
    },
    // 50% грусти
    {
        "My beloved dog was put down last week. It was very hard to see her weak and tired — my heart ached with pain.",
        "I lost my job after many years of service. I feel empty — it seems like nothing can be returned.",
        "My parents divorced recently. Inside remains emptiness and a feeling of losing the family."
    },
    // 60% грусти
    {
        "My girlfriend left me for another. I feel broken — my heart hurts every time I remember her.",
        "I learned about a serious illness of a friend. Inside, it feels like I'm torn by pain — very hard to see his suffering.",
        "I lost a close person in an accident. The whole world seemed to collapse — hard to find strength to move on."
    },
    // 70% грусти
    {
        "My mother has cancer. Every day is filled with fear and anticipation of bad news — my heart tightens with pain.",
        "I went through a divorce after many years of marriage. The feeling of emptiness and loneliness never leaves me.",
        "I was fired from work at the worst possible moment. Inside, there is a storm — it feels like the whole world is against me."
    },
    // 80% грусти
    {
        "My sister was buried yesterday. I feel empty — inside it feels like pain of loss is tearing me apart.",
        "I lost my house to a fire. All memories burned with it — my heart is full of bitterness and hopelessness.",
        "A close person passed away after a long battle with illness. Inside, a storm rages — it feels like there is no hope left."
    },
    // 90% грусти
    {
        "I lost my job and house in one month. Inside, a storm rages — it feels like the whole world is against me, and hope is almost gone.",
        "My family broke apart forever after a scandal and betrayal. A feeling of hopelessness completely overwhelms me.",
        "My beloved left me after many years together. Left alone in this world — heart broken forever."
    }
};
const char* backstory_list_angry[10][3] = {
    // 0% злости
    {
        "Bartender, I just came to drink and relax. Everything is calm, nothing makes me angry.",
        "Everything was calm, I'm not angry at anyone. Just want a little rest after work.",
        "No reasons for anger. Today went smoothly, I'm satisfied."
    },
    // 10% злости
    {
        "My friend's car was recently hit, and I'm a bit angry at that driver, though I try to keep myself calm.",
        "I took a loan for home repairs, this situation annoys me a little, but I try not to show emotions.",
        "Problems with neighbors annoy me a bit, but I hold myself back and don't show it."
    },
    // 20% злости
    {
        "I recently had to pay off debts — that makes me a little angry, I feel inner tension.",
        "The car broke down at the worst possible moment, and I got a bit mad at fate.",
        "Work dragged on late, and I feel slight anger because of it."
    },
    // 30% злости
    {
        "I got angry that my neighbor made noise again at night — I feel inner rage.",
        "I took a loan for my son's education and am a bit angry at my inability to solve everything quickly.",
        "When I had to wait long in a queue — I felt irritation and anger."
    },
    // 40% злости
    {
        "The car broke down again at the worst moment — I feel strong irritation and anger.",
        "I had to pay a large fine for a violation — anger boils inside.",
        "When I was denied a loan without explanation — I felt fury."
    },
    // 50% злости
    {
        "It really angers me that my business is losing money because of competitors — rage is raging inside.",
        "I was recently fired from work without explanation — I feel strong anger and disappointment.",
        "When my friend deceived me — everything boils inside with fury."
    },
    // 60% злости
    {
        "I'm so angry at myself for not paying the debt on time — rage is raging inside.",
        "My car broke down again, and money for repairs ran out — I feel strong anger.",
        "When my wife forgot my birthday — irritation and fury boil inside."
    },
    // 70% злости
    {
        "It pisses me off that everyone around ignores my problems — fury is raging inside.",
        "I took a loan with high interest and feel strong irritation because of this burden.",
        "When I was unfairly accused — everything boils inside with anger."
    },
    // 80% злости
    {
        "I'm so angry at my inability to solve the debt problem — fury is raging inside.",
        "The car completely broke down before an important trip — I feel strong anger and irritation.",
        "When I was hurt by close people — everything boils inside with fury."
    },
    // 100% злости
    {
        "I'm so angry at the whole world for what happened: my child's car was hit. Everything inside explodes with rage!",
        "I'm overwhelmed with anger: I took a loan to pay debts, but now everything is just getting worse!",
        "I'm completely enraged: all my efforts were in vain, and I can't take it anymore!"
    }
};

const char* backstory_list_scared[10][3] = {
    // 0% испуга
    {
        "I was walking home calmly, as usual. No incidents — just another day. I felt confident and calm.",
        "Everything was quiet and peaceful. I didn't even notice anything unusual, just walking down the street enjoying the evening.",
        "Life went on as usual. No worries or dangers — everything was normal."
    },
    // 10% испуга
    {
        "I was walking down the street when a car passed very close, almost hit me. I was a little scared but quickly recovered.",
        "Yesterday at the intersection I almost got hit by a car. I was so scared my heart raced for a moment.",
        "When I heard a car horn so close, I almost fainted from fear."
    },
    // 20% испуга
    {
        "I took a loan at the bank and was a little worried about the payments. Inside, I was anxious but managed.",
        "When the bank called me reminding about the debt, my heart raced — I was a little scared of possible problems.",
        "I took a loan for home repairs — a bit nervous, afraid I might not pay on time."
    },
    // 30% испуга
    {
        "One night I heard strange sounds outside the window. I was scared to the point of shaking — thought someone had broken into the house.",
        "Yesterday evening I heard noise behind the door and felt strong fear — maybe it was someone dangerous.",
        "When I saw a shadow at the window in the dark, my heart raced — I was very frightened."
    },
    // 40% испуга
    {
        "I accidentally took a loan larger than I could repay and started to panic. The fear of debts was very strong.",
        "When I was told about a possible job loss, I felt strong fear for the future and debts.",
        "Suddenly learned about a serious health problem — fear overwhelmed me completely."
    },
    // 50% испуга
    {
        "A car sped past me so close I almost fell. The fright was sudden and strong.",
        "When I was informed about a serious accident of my friend, I felt strong fear for his life.",
        "I found out about an upcoming court case — my heart raced with fear and anxiety."
    },
    // 60% испуга
    {
        "I was robbed at night on the street. The fear was so strong I nearly lost consciousness.",
        "When I learned that I had serious debt problems impossible to pay — fear completely took over.",
        "Felt horror at the thought of possibly losing all my property because of debts."
    },
    // 70% испуга
    {
        "A car hit my friend right in front of me. I was in shock and very scared — it felt like everything inside me collapsed.",
        "I learned about a serious illness of a close person — fear for their life literally paralyzed me.",
        "I was detained by law enforcement by mistake — fear of losing freedom was very strong."
    },
    // 80% испуга
    {
        "I was trapped in a building during a fire. Panic overwhelmed me — it felt like I would suffocate from fear.",
        "Received threat of dismissal or serious consequences at work — fear for the future was unbearable.",
        "I was almost killed during a bank robbery — horror completely took over."
    },
    // 90% испуга
    {
        "A car hit me at full speed on a crosswalk. I felt death nearby and was very scared to the point of shaking.",
        "I learned about an upcoming court trial accused of a crime — fear of losing everything was incredible.",
        "I was almost killed during an armed attack — horror gripped every cell of my body."
    }
};

#endif