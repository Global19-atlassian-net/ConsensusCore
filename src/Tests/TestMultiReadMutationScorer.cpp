// Copyright (c) 2011-2013, Pacific Biosciences of California, Inc.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted (subject to the limitations in the
// disclaimer below) provided that the following conditions are met:
//
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//  * Redistributions in binary form must reproduce the above
//    copyright notice, this list of conditions and the following
//    disclaimer in the documentation and/or other materials provided
//    with the distribution.
//
//  * Neither the name of Pacific Biosciences nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
// GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY PACIFIC
// BIOSCIENCES AND ITS CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL PACIFIC BIOSCIENCES OR ITS
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
// USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.

// Author: David Alexander

#include <gtest/gtest.h>
#include <boost/assign.hpp>
#include <string>
#include <vector>

#include "Quiver/MultiReadMutationScorer.hpp"
#include "Quiver/QuiverConfig.hpp"
#include "Quiver/SimpleRecursor.hpp"
#include "Quiver/SseRecursor.hpp"
#include "Quiver/ReadScorer.hpp"

#include "ParameterSettings.hpp"

using namespace ConsensusCore;  // NOLINT
using namespace boost::assign;  // NOLINT


//
// Tests for supporting code: OrientedMutation, ReadScoresMutation
//

TEST(MutationOrientationTests, ReadScoresMutation1)
{
    //  012345678901
    //    >>>>>>>>    mr
    QvSequenceFeatures f("G");
    MappedRead mr(f, FORWARD_STRAND,  2, 10);

    for (int p=0; p <= 11; p++)
    {
        Mutation mSubs(SUBSTITUTION, p, 'G');
        Mutation mDel (DELETION, p, '-');
        Mutation mIns (INSERTION, p, 'G');

        if (p < mr.TemplateStart)
        {
            EXPECT_TRUE(!ReadScoresMutation(mr, mSubs) &&
                        !ReadScoresMutation(mr, mDel)  &&
                        !ReadScoresMutation(mr, mIns));
        }
        else if (p == mr.TemplateStart)
        {
            EXPECT_TRUE( ReadScoresMutation(mr, mSubs) &&
                         ReadScoresMutation(mr, mDel)  &&
                        !ReadScoresMutation(mr, mIns));
        }
        else if (p < mr.TemplateEnd)
        {
            EXPECT_TRUE( ReadScoresMutation(mr, mSubs) &&
                         ReadScoresMutation(mr, mDel)  &&
                         ReadScoresMutation(mr, mIns));
        }
        else if (p == mr.TemplateEnd)
        {
            EXPECT_TRUE(!ReadScoresMutation(mr, mSubs) &&
                        !ReadScoresMutation(mr, mDel)  &&
                         ReadScoresMutation(mr, mIns));

        }
        else if (p > mr.TemplateEnd)
        {
            EXPECT_TRUE(!ReadScoresMutation(mr, mSubs) &&
                        !ReadScoresMutation(mr, mDel)  &&
                        !ReadScoresMutation(mr, mIns));
        }
    }
}


TEST(MutationOrientationTests, ReadScoresMutation2)
{
    //  012345678901
    //    >>>>>>>>    mr
    QvSequenceFeatures f("G");
    MappedRead mr(f, FORWARD_STRAND,  2, 10);

    for (int p=0; p <= 11; p++)
    {
        Mutation mSubs2(SUBSTITUTION, p, p+2, "GG");
        Mutation mDel2 (DELETION,     p, p+2, "");
        if (p >= 1 && p <= 9)
        {
            EXPECT_TRUE( ReadScoresMutation(mr, mSubs2) &&
                         ReadScoresMutation(mr, mDel2));
        }
        else
        {
            EXPECT_TRUE(!ReadScoresMutation(mr, mSubs2) &&
                        !ReadScoresMutation(mr, mDel2));
        }
    }
}



TEST(MutationOrientationTests, OrientedMutation)
{
    //  012345678901
    //    >>>>>>>>    mr1
    //    <<<<<<<<    mr2
    QvSequenceFeatures f1("G"), f2("G");
    MappedRead mr1(f1, FORWARD_STRAND, 2, 10);
    MappedRead mr2(f2, REVERSE_STRAND, 2, 10);

    for (int p=2; p <= 9; p++)
    {
        Mutation mSubs (SUBSTITUTION, p, 'G');
        Mutation mDel  (DELETION, p, '-');

        EXPECT_EQ(Mutation(SUBSTITUTION, p-mr1.TemplateStart, 'G'), OrientedMutation(mr1, mSubs));
        EXPECT_EQ(Mutation(DELETION,     p-mr1.TemplateStart, '-'), OrientedMutation(mr1, mDel));
        EXPECT_EQ(Mutation(SUBSTITUTION, mr2.TemplateEnd-1-p, 'C'), OrientedMutation(mr2, mSubs));
        EXPECT_EQ(Mutation(DELETION,     mr2.TemplateEnd-1-p, '-'), OrientedMutation(mr2, mDel));
    }

    for (int p=3; p <= 10; p++)
    {
        Mutation mIns  (INSERTION, p, 'G');
        Mutation mIns2 (INSERTION, p, p, "GT");

        EXPECT_EQ(Mutation(INSERTION, p-mr1.TemplateStart, 'G'),
                  OrientedMutation(mr1, mIns));
        EXPECT_EQ(Mutation(INSERTION, p-mr1.TemplateStart, p-mr1.TemplateStart, "GT"),
                  OrientedMutation(mr1, mIns2));
        EXPECT_EQ(Mutation(INSERTION, mr2.TemplateEnd-p, 'C'),
                  OrientedMutation(mr2, mIns));
        EXPECT_EQ(Mutation(INSERTION, mr2.TemplateEnd-p, mr2.TemplateEnd-p, "AC"),
                  OrientedMutation(mr2, mIns2));

    }

    for (int p=1; p <= 9; p++)
    {
        Mutation mSubs2(SUBSTITUTION, p, p+2, "GG");
        Mutation mDel2 (DELETION,     p, p+2, "");

        if (p == 1)
        {
            EXPECT_EQ(Mutation(SUBSTITUTION, 0, 1, "G"),    OrientedMutation(mr1, mSubs2));
            EXPECT_EQ(Mutation(DELETION,     0, 1, ""),     OrientedMutation(mr1, mDel2));
            EXPECT_EQ(Mutation(SUBSTITUTION, 7, 8, "C"),    OrientedMutation(mr2, mSubs2));
            EXPECT_EQ(Mutation(DELETION,     7, 8, ""),     OrientedMutation(mr2, mDel2));
        }
        else if (p == 9)
        {
            EXPECT_EQ(Mutation(SUBSTITUTION, 7, 8, "G"),    OrientedMutation(mr1, mSubs2));
            EXPECT_EQ(Mutation(DELETION,     7, 8, ""),     OrientedMutation(mr1, mDel2));
            EXPECT_EQ(Mutation(SUBSTITUTION, 0, 1, "C"),    OrientedMutation(mr2, mSubs2));
            EXPECT_EQ(Mutation(DELETION,     0, 1, ""),     OrientedMutation(mr2, mDel2));
        }
        else
        {
            EXPECT_EQ(Mutation(SUBSTITUTION, p-2, p, "GG"), OrientedMutation(mr1, mSubs2));
            EXPECT_EQ(Mutation(DELETION,     p-2, p, ""),   OrientedMutation(mr1, mDel2));
            EXPECT_EQ(Mutation(SUBSTITUTION, mr2.TemplateEnd-p-2, mr2.TemplateEnd-p,  "CC"),
                      OrientedMutation(mr2, mSubs2));
            EXPECT_EQ(Mutation(DELETION, mr2.TemplateEnd-p-2, mr2.TemplateEnd-p, ""),
                      OrientedMutation(mr2, mDel2));
        }
    }
}


//
//  Tests for the multi read mutation scorer itself
//

TYPED_TEST_CASE(MultiReadMutationScorerTest, testing::Types<SparseSseQvRecursor>);

template <typename R>
class MultiReadMutationScorerTest : public testing::Test
{
public:
    typedef MultiReadMutationScorer<R> MMS;

protected:
    MultiReadMutationScorerTest()
        : testingConfig_(TestingParams<typename MMS::EvaluatorType::ParamsType>(),
                         ALL_MOVES,
                         BandingOptions(4, 200),
                         -500)
    {}

    virtual ~MultiReadMutationScorerTest() {}

protected:
    QuiverConfig testingConfig_;
};


#define MMS MultiReadMutationScorer<TypeParam>
#define MS  MutationScorer<TypeParam>
#define E   typename TypeParam::EvaluatorType

#define config    (this->testingConfig_)
#define params    (this->testingConfig_.QvParams)


TYPED_TEST(MultiReadMutationScorerTest, Template)
{
    //                 0123456789
    std::string fwd = "AAAATTTTGG";
    std::string rev = ReverseComplement(fwd);

    // Make sure the Template function works right
    MMS mScorer(this->testingConfig_, fwd);
    ASSERT_EQ(fwd, mScorer.Template());
    ASSERT_EQ(fwd, mScorer.Template(FORWARD_STRAND));
    ASSERT_EQ(rev, mScorer.Template(REVERSE_STRAND));
    ASSERT_EQ(fwd, mScorer.Template(FORWARD_STRAND, 0, 10));
    ASSERT_EQ(rev, mScorer.Template(REVERSE_STRAND, 0, 10));
    ASSERT_EQ("AT", mScorer.Template(FORWARD_STRAND, 3, 5));
    ASSERT_EQ("AT", mScorer.Template(REVERSE_STRAND, 3, 5));
    ASSERT_EQ("TTTT", mScorer.Template(FORWARD_STRAND, 4, 8));
    ASSERT_EQ("AAAA", mScorer.Template(REVERSE_STRAND, 4, 8));
}

TYPED_TEST(MultiReadMutationScorerTest, BasicTest)
{
    std::string tpl = "TTGATTACATT";
    std::string revTpl = ReverseComplement(tpl);

    MMS mScorer(this->testingConfig_, tpl);
    mScorer.AddRead(QvSequenceFeatures("TTGATTACATT"), FORWARD_STRAND);

    Mutation noOpMutation(SUBSTITUTION, 6, 'A');
    Mutation insertMutation(INSERTION, 6, 'A');
    Mutation substitutionMutation(SUBSTITUTION, 6, 'T');
    Mutation deletionMutation(DELETION, 6, '-');

    EXPECT_EQ(0, mScorer.Score(noOpMutation));
    EXPECT_EQ("TTGATTACATT", mScorer.Template());
    EXPECT_EQ(params.Merge[0], mScorer.Score(insertMutation));
    EXPECT_EQ("TTGATTACATT", mScorer.Template());
    EXPECT_EQ(params.Mismatch, mScorer.Score(substitutionMutation));
    EXPECT_EQ("TTGATTACATT", mScorer.Template());
    EXPECT_EQ(params.Nce, mScorer.Score(deletionMutation));
    EXPECT_EQ("TTGATTACATT", mScorer.Template());

    mScorer.AddRead(QvSequenceFeatures("TTGATTACATT"), FORWARD_STRAND);

    EXPECT_EQ(0, mScorer.Score(noOpMutation));
    EXPECT_EQ("TTGATTACATT", mScorer.Template());
    EXPECT_EQ(-4, mScorer.Score(insertMutation));
    EXPECT_EQ("TTGATTACATT", mScorer.Template());
    EXPECT_EQ(-20, mScorer.Score(substitutionMutation));
    EXPECT_EQ("TTGATTACATT", mScorer.Template());
    EXPECT_EQ(-16, mScorer.Score(deletionMutation));
    EXPECT_EQ("TTGATTACATT", mScorer.Template());

    std::vector<Mutation*> muts;
    muts += &insertMutation;
    mScorer.ApplyMutations(muts);
    EXPECT_EQ("TTGATTAACATT", mScorer.Template());

    Mutation newNoOpMutation(SUBSTITUTION, 6, 'A');
    EXPECT_EQ(0, mScorer.Score(newNoOpMutation));
}


TYPED_TEST(MultiReadMutationScorerTest, ManyMutationTest)
{
    std::string tpl = "TTGACGTACGTGTGACACAGTACAGATTACAAACCGGTAGACATTACATT";
    std::string revTpl = ReverseComplement(tpl);

    MMS mScorer(this->testingConfig_, tpl);
    mScorer.AddRead(QvSequenceFeatures("TTGATTACATT"), FORWARD_STRAND);


    std::vector<Mutation*> muts;
    for (int i = 0; i < tpl.length(); i+=2)
    {
        Mutation* mutation = new Mutation(SUBSTITUTION, i, 'A');
        muts += mutation;
    }

    mScorer.ApplyMutations(muts);
    EXPECT_EQ(tpl.length(), mScorer.Template().length());

    foreach (Mutation* mut, muts)
    {
        delete mut;
    }
}



TYPED_TEST(MultiReadMutationScorerTest, CopyConstructorTest)
{
    std::string tpl = "TTGATTACATT";
    std::string revTpl = ReverseComplement(tpl);

    MMS mScorer(this->testingConfig_, tpl);
    mScorer.AddRead(QvSequenceFeatures("TTGATTACATT"), FORWARD_STRAND);

    // Run the copy constructor of MultiReadMutationScorer
    MMS mCopy(mScorer);

    Mutation noOpMutation(SUBSTITUTION, 6, 'A');
    Mutation insertMutation(INSERTION, 6, 'A');
    Mutation substitutionMutation(SUBSTITUTION, 6, 'T');
    Mutation deletionMutation(DELETION, 6, '-');

    EXPECT_EQ(0, mScorer.Score(noOpMutation));
    EXPECT_EQ("TTGATTACATT", mScorer.Template());
    EXPECT_EQ(params.Merge[0], mScorer.Score(insertMutation));
    EXPECT_EQ("TTGATTACATT", mScorer.Template());
    EXPECT_EQ(params.Mismatch, mScorer.Score(substitutionMutation));
    EXPECT_EQ("TTGATTACATT", mScorer.Template());
    EXPECT_EQ(params.Nce, mScorer.Score(deletionMutation));
    EXPECT_EQ("TTGATTACATT", mScorer.Template());

    // Apply mutation to copy
    std::vector<Mutation*> muts;
    muts += &insertMutation;
    mCopy.ApplyMutations(muts);

    // copy template should change
    EXPECT_EQ("TTGATTAACATT", mCopy.Template());

    // original should be unchanged
    EXPECT_EQ("TTGATTACATT", mScorer.Template());

    // Score of original shouldn't change
    EXPECT_EQ(0, mScorer.Score(noOpMutation));
    EXPECT_EQ("TTGATTACATT", mScorer.Template());
    EXPECT_EQ(params.Merge[0], mScorer.Score(insertMutation));
    EXPECT_EQ("TTGATTACATT", mScorer.Template());
    EXPECT_EQ(params.Mismatch, mScorer.Score(substitutionMutation));
    EXPECT_EQ("TTGATTACATT", mScorer.Template());
    EXPECT_EQ(params.Nce, mScorer.Score(deletionMutation));
    EXPECT_EQ("TTGATTACATT", mScorer.Template());
}



TYPED_TEST(MultiReadMutationScorerTest, ReverseStrandTest)
{
    // Just make sure if we reverse complemented the universe,
    // everything would come out the same.
    std::string tpl = "AATGTAATCAA";
    MMS mScorer(this->testingConfig_, tpl);
    mScorer.AddRead(QvSequenceFeatures("TTGATTACATT"), REVERSE_STRAND);

    Mutation noOpMutation(SUBSTITUTION, 4, 'T');
    Mutation insertMutation(INSERTION, 5, 'T');
    Mutation substitutionMutation(SUBSTITUTION, 4, 'A');
    Mutation deletionMutation(DELETION, 4, '-');

    EXPECT_EQ(0               , mScorer.Score(noOpMutation));
    EXPECT_EQ("AATGTAATCAA"   , mScorer.Template());
    EXPECT_EQ(params.Merge[0]    , mScorer.Score(insertMutation));
    EXPECT_EQ("AATGTAATCAA"   , mScorer.Template());
    EXPECT_EQ(params.Mismatch , mScorer.Score(substitutionMutation));
    EXPECT_EQ("AATGTAATCAA"   , mScorer.Template());
    EXPECT_EQ(params.Nce      , mScorer.Score(deletionMutation));
    EXPECT_EQ("AATGTAATCAA"   , mScorer.Template());

    mScorer.AddRead(QvSequenceFeatures("TTGATTACATT"), REVERSE_STRAND);

    EXPECT_EQ(0                 , mScorer.Score(noOpMutation));
    EXPECT_EQ("AATGTAATCAA"     , mScorer.Template());
    EXPECT_EQ(2*params.Merge[0] , mScorer.Score(insertMutation));
    EXPECT_EQ("AATGTAATCAA"     , mScorer.Template());
    EXPECT_EQ(2*params.Mismatch , mScorer.Score(substitutionMutation));
    EXPECT_EQ("AATGTAATCAA"     , mScorer.Template());
    EXPECT_EQ(2*params.Nce      , mScorer.Score(deletionMutation));
    EXPECT_EQ("AATGTAATCAA"     , mScorer.Template());

    std::vector<Mutation*> muts;
    muts += &insertMutation;
    mScorer.ApplyMutations(muts);
    EXPECT_EQ("AATGTTAATCAA", mScorer.Template());

    Mutation newNoOpMutation(SUBSTITUTION, 4, 'T');
    EXPECT_EQ(0, mScorer.Score(newNoOpMutation));
}


TYPED_TEST(MultiReadMutationScorerTest, TestMutationsAtBeginning)
{
    std::string tpl = "TTGATTACATT";

    MMS mScorer(this->testingConfig_, tpl);
    mScorer.AddRead(QvSequenceFeatures("TTGATTACATT"), FORWARD_STRAND);

    Mutation noOpMutation     (SUBSTITUTION , 0, 'T');
    Mutation deletionMutation (DELETION     , 0, '-');
    Mutation insertMutation   (INSERTION    , 0, 'A');
    Mutation insertMutation2  (INSERTION    , 1, 'A');

    EXPECT_EQ(0                , mScorer.Score(noOpMutation));

    // Note that there is no actual way to test an insertion before
    // the first base ... the alignment just slides over.
    EXPECT_EQ(0                , mScorer.Score(insertMutation));
    EXPECT_EQ(params.DeletionN , mScorer.Score(insertMutation2));
    EXPECT_EQ(params.Branch    , mScorer.Score(deletionMutation));  // now there is a branch...
}

TYPED_TEST(MultiReadMutationScorerTest, TestMutationsAtEnd)
{
    //                 01234567890
    std::string tpl = "TTGATTACATT";

    MMS mScorer(this->testingConfig_, tpl);
    mScorer.AddRead(QvSequenceFeatures("TTGATTACATT"), FORWARD_STRAND);

    Mutation noOpMutation     (SUBSTITUTION , 10, 'T');
    Mutation deletionMutation (DELETION     , 10, '-');
    Mutation insertMutation   (INSERTION    , 11, 'A');
    Mutation insertMutation2  (INSERTION    , 12, 'A');

    EXPECT_EQ(0                , mScorer.Score(noOpMutation));

    // Note that there is no actual way to test an insertion before
    // the first base ... the alignment just slides over.
    EXPECT_EQ(params.DeletionN , mScorer.Score(insertMutation));
    EXPECT_EQ(0                , mScorer.Score(insertMutation2));
    EXPECT_EQ(params.Branch    , mScorer.Score(deletionMutation));
}




TYPED_TEST(MultiReadMutationScorerTest, NonSpanningReadsTest1)
{
    // read1:                     >>>>>>>>>>>
    // read2:          <<<<<<<<<<<
    //                 0123456789012345678901
    std::string tpl = "AATGTAATCAATTGATTACATT";
    MMS mScorer(this->testingConfig_, tpl);

    // mutations in the latter half
    Mutation noOpMutation1(SUBSTITUTION, 17, 'A');
    Mutation insertMutation1(INSERTION, 17, 'A');
    Mutation substitutionMutation1(SUBSTITUTION, 17, 'T');
    Mutation deletionMutation1(DELETION, 17, '-');

    // mutations in the first half
    Mutation noOpMutation2(SUBSTITUTION, 4, 'T');
    Mutation insertMutation2(INSERTION, 5, 'T');
    Mutation substitutionMutation2(SUBSTITUTION, 4, 'A');
    Mutation deletionMutation2(DELETION, 4, '-');

    mScorer.AddRead(QvSequenceFeatures("TTGATTACATT"), FORWARD_STRAND, 11, 22);
    mScorer.AddRead(QvSequenceFeatures("TTGATTACATT"), REVERSE_STRAND,  0, 11);

    EXPECT_EQ(0               , mScorer.Score(noOpMutation1));
    EXPECT_EQ(params.Merge[0] , mScorer.Score(insertMutation1));
    EXPECT_EQ(params.Mismatch , mScorer.Score(substitutionMutation1));
    EXPECT_EQ(params.Nce      , mScorer.Score(deletionMutation1));

    EXPECT_EQ(0               , mScorer.Score(noOpMutation2));
    EXPECT_EQ(params.Merge[0] , mScorer.Score(insertMutation2));
    EXPECT_EQ(params.Mismatch , mScorer.Score(substitutionMutation2));
    EXPECT_EQ(params.Nce      , mScorer.Score(deletionMutation2));

    EXPECT_EQ(tpl, mScorer.Template());

    std::vector<Mutation*> muts;
    muts += &insertMutation1, &insertMutation2;
    mScorer.ApplyMutations(muts);
    EXPECT_EQ("AATGTTAATCAATTGATTAACATT", mScorer.Template());
}


TYPED_TEST(MultiReadMutationScorerTest, CopyTest)
{
    // read1:                     >>>>>>>>>>>
    // read2:          <<<<<<<<<<<
    //                 0123456789012345678901
    std::string tpl = "AATGTAATCAATTGATTACATT";
    MMS mScorer(this->testingConfig_, tpl);
    mScorer.AddRead(QvSequenceFeatures("TTGATTACATT"), FORWARD_STRAND, 11, 22);
    mScorer.AddRead(QvSequenceFeatures("TTGATTACATT"), REVERSE_STRAND,  0, 11);
    MMS mScorerCopy(mScorer);

    ASSERT_EQ(mScorer.BaselineScore(), mScorerCopy.BaselineScore());
}


TYPED_TEST(MultiReadMutationScorerTest, MultiBaseIndelsAtBounds)
{
    // read1:                     >>>>>>>>>
    // read2:            <<<<<<<<<
    //                 0123456789012345678901
    std::string tpl = "AATGTAATCAATTGATTACATT";



}